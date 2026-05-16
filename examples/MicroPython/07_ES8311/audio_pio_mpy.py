import rp2
from machine import Pin
import time

try:
    from rp2 import DMA
except ImportError:
    DMA = None

# ES8311 is used as I2S master. This tiny PIO program only provides MCLK.
@rp2.asm_pio(set_init=rp2.PIO.OUT_HIGH)
def mclk_pio():
    label("loop")
    set(pins, 1)
    set(pins, 0)
    jmp("loop")

@rp2.asm_pio(out_init=rp2.PIO.OUT_HIGH,
             out_shiftdir=rp2.PIO.SHIFT_LEFT,
             fifo_join=rp2.PIO.JOIN_TX)
def audio_pio_out():
    # Transmit one 32-bit stereo frame per pull: left 16 bits, then right 16 bits.
    pull()
    wait(0, gpio, 5)
    label("start")
    pull()
    wait(1, gpio, 5)
    wait(1, gpio, 4)
    set(x, 15)
    label("left_loop")
    wait(0, gpio, 4)
    out(pins, 1)
    wait(1, gpio, 4)
    jmp(x_dec, "left_loop")
    wait(0, gpio, 5)
    wait(1, gpio, 4)
    set(x, 15)
    label("right_loop")
    wait(0, gpio, 4)
    out(pins, 1)
    wait(1, gpio, 4)
    jmp(x_dec, "right_loop")
    jmp("start")

@rp2.asm_pio(in_shiftdir=rp2.PIO.SHIFT_LEFT,
             autopush=True,
             push_thresh=16,
             fifo_join=rp2.PIO.JOIN_RX)
def audio_pio_in():
    # Capture only one 16-bit channel. Pushing 16-bit samples keeps DMA aligned
    # and avoids random half-word selection from a 32-bit stereo FIFO word.
    wait(1, gpio, 5)
    label("start")
    wait(0, gpio, 5)
    wait(1, gpio, 4)
    set(x, 15)
    label("left_loop")
    wait(0, gpio, 4)
    wait(1, gpio, 4)
    in_(pins, 1)
    jmp(x_dec, "left_loop")
    wait(1, gpio, 5)
    jmp("start")


class AudioPIO:
    def __init__(self,
                 mclk_pin=3,
                 dout_pin=1,
                 din_pin=2,
                 lrclk_pin=5,
                 bclk_pin=4,
                 sm_dout_id=0,
                 sm_din_id=5,
                 sm_mclk_id=2):
        self.mclk_pin = mclk_pin
        self.dout_pin = dout_pin
        self.din_pin = din_pin
        self.lrclk_pin = lrclk_pin
        self.bclk_pin = bclk_pin
        self.sm_dout_id = sm_dout_id
        self.sm_din_id = sm_din_id
        self.sm_mclk_id = sm_mclk_id
        self.mclk_freq = 24000 * 256
        self.sample_freq = 24000
        self.channel_count = 1
        self.rx_channel = 0
        self.sm_mclk = None
        self.sm_dout = None
        self.sm_din = None
        self.dma_rx = None
        self.dma_tx = None

    def mclk_pio_init(self):
        Pin(self.mclk_pin, Pin.OUT)
        self.sm_mclk = rp2.StateMachine(
            self.sm_mclk_id,
            mclk_pio,
            freq=self.mclk_freq * 3,
            set_base=Pin(self.mclk_pin)
        )
        self.sm_mclk.active(0)
        self.sm_mclk.active(1)

    def dout_pio_init(self):
        # BCLK and LRCLK are driven by ES8311, so PIO waits on them as inputs.
        Pin(self.dout_pin, Pin.OUT)
        Pin(self.bclk_pin, Pin.IN)
        Pin(self.lrclk_pin, Pin.IN)
        self.sm_dout = rp2.StateMachine(
            self.sm_dout_id,
            audio_pio_out,
            freq=12288000,
            out_base=Pin(self.dout_pin)
        )
        self.sm_dout.active(0)
        try:
            self.sm_dout.restart()
        except AttributeError:
            pass
        self.sm_dout.active(1)

    def din_pio_init(self):
        # Explicit input setup is important on MicroPython; it mirrors the C SDK
        # pio_gpio_init()/pindirs setup used by the reference firmware.
        Pin(self.din_pin, Pin.IN)
        Pin(self.bclk_pin, Pin.IN)
        Pin(self.lrclk_pin, Pin.IN)
        self.sm_din = rp2.StateMachine(
            self.sm_din_id,
            audio_pio_in,
            freq=12288000,
            in_base=Pin(self.din_pin)
        )
        self.sm_din.active(0)
        try:
            self.sm_din.restart()
        except AttributeError:
            pass
        self.sm_din.active(1)

    def audio_out(self, samples):
        if self.channel_count == 1:
            for s in samples:
                self.sm_dout.put((s & 0xFFFF) << 16)
        else:
            for s in samples:
                v = (s & 0xFFFF)
                self.sm_dout.put((v << 16) | v)

    def audio_in(self, count):
        from array import array
        buf = array("h", [0] * count)
        self.audio_in_into(buf)
        return buf

    def audio_in_into(self, buf):
        for i in range(len(buf)):
            t0 = time.ticks_ms()
            while self.sm_din.rx_fifo() == 0:
                if time.ticks_diff(time.ticks_ms(), t0) > 500:
                    raise RuntimeError("audio_in timeout: no RX data (check ES8311 BCLK/LRCLK and DIN wiring)")
            v = self.sm_din.get() & 0xFFFF
            if v & 0x8000:
                v -= 0x10000
            buf[i] = v

    def audio_in_words(self, count):
        from array import array
        buf = array("I", [0] * count)
        for i in range(count):
            t0 = time.ticks_ms()
            while self.sm_din.rx_fifo() == 0:
                if time.ticks_diff(time.ticks_ms(), t0) > 500:
                    raise RuntimeError("audio_in_words timeout: no RX data")
            buf[i] = self.sm_din.get()
        return buf

    def audio_out_from(self, buf):
        self.audio_out(buf)

    def audio_out_i16(self, in_buf, out_buf32):
        n = len(in_buf)
        if len(out_buf32) < n:
            raise ValueError("out_buf32 too small")
        if self.channel_count == 1:
            for i in range(n):
                out_buf32[i] = (in_buf[i] & 0xFFFF) << 16
        else:
            for i in range(n):
                v = in_buf[i] & 0xFFFF
                out_buf32[i] = (v << 16) | v

        try:
            self.sm_dout.put(out_buf32, 0)
        except TypeError:
            for i in range(n):
                self.sm_dout.put(out_buf32[i])

    def _pio_dreq(self, sm_id, is_tx):
        # MicroPython StateMachine IDs 0..3 map to PIO0, 4..7 map to PIO1.
        pio_num = sm_id // 4
        sm_num = sm_id & 3
        return (pio_num << 3) + sm_num + (0 if is_tx else 4)

    def dma_supported(self):
        return DMA is not None

    def _restart_rx(self, discard_words=4):
        # Start each capture from a clean FIFO and a fresh PIO shift state.
        self.sm_din.active(0)
        try:
            self.sm_din.restart()
        except AttributeError:
            pass
        while self.sm_din.rx_fifo():
            self.sm_din.get()
        self.sm_din.active(1)
        for _ in range(discard_words):
            t0 = time.ticks_ms()
            while self.sm_din.rx_fifo() == 0:
                if time.ticks_diff(time.ticks_ms(), t0) > 50:
                    return
            self.sm_din.get()

    def _restart_tx(self):
        # Restarting TX avoids stale FIFO/shift-register state between clips.
        self.sm_dout.active(0)
        try:
            self.sm_dout.restart()
        except AttributeError:
            pass
        self.sm_dout.active(1)

    def _dma_pack_ctrl(self, dma, **kwargs):
        try:
            return dma.pack_ctrl(**kwargs)
        except TypeError:
            kwargs.pop("bswap", None)
            try:
                return dma.pack_ctrl(**kwargs)
            except TypeError:
                kwargs.pop("high_pri", None)
                return dma.pack_ctrl(**kwargs)

    def dma_record_into(self, buf):
        # Real-time capture path: PIO RX FIFO -> int16 sample buffer.
        if DMA is None:
            raise RuntimeError("rp2.DMA is not available in this MicroPython firmware")
        if self.dma_rx is None:
            self.dma_rx = DMA()
        self._restart_rx()
        ctrl = self._dma_pack_ctrl(
            self.dma_rx,
            size=1,
            inc_read=False,
            inc_write=True,
            treq_sel=self._pio_dreq(self.sm_din_id, False),
            high_pri=True,
            bswap=False
        )
        self.dma_rx.config(
            read=self.sm_din,
            write=buf,
            count=len(buf),
            ctrl=ctrl,
            trigger=True
        )
        while self.dma_rx.active():
            pass

    def dma_record_words_into(self, buf):
        # Debug helper only. Normal loopback uses dma_record_into() with the
        # 16-bit mono RX PIO program above.
        if DMA is None:
            raise RuntimeError("rp2.DMA is not available in this MicroPython firmware")
        if self.dma_rx is None:
            self.dma_rx = DMA()
        self._restart_rx()
        ctrl = self._dma_pack_ctrl(
            self.dma_rx,
            size=2,
            inc_read=False,
            inc_write=True,
            treq_sel=self._pio_dreq(self.sm_din_id, False),
            high_pri=True,
            bswap=False
        )
        self.dma_rx.config(
            read=self.sm_din,
            write=buf,
            count=len(buf),
            ctrl=ctrl,
            trigger=True
        )
        while self.dma_rx.active():
            pass

    def dma_play_from_i16(self, buf):
        # Real-time playback path: int16 sample buffer -> PIO TX FIFO.
        if DMA is None:
            raise RuntimeError("rp2.DMA is not available in this MicroPython firmware")
        if self.dma_tx is None:
            self.dma_tx = DMA()
        self._restart_tx()
        ctrl = self._dma_pack_ctrl(
            self.dma_tx,
            size=1,
            inc_read=True,
            inc_write=False,
            treq_sel=self._pio_dreq(self.sm_dout_id, True),
            high_pri=True,
            bswap=False
        )
        self.dma_tx.config(
            read=buf,
            write=self.sm_dout,
            count=len(buf),
            ctrl=ctrl,
            trigger=True
        )
        while self.dma_tx.active():
            pass

    def dma_play_words(self, buf):
        # Debug helper for pre-packed 32-bit stereo words.
        if DMA is None:
            raise RuntimeError("rp2.DMA is not available in this MicroPython firmware")
        if self.dma_tx is None:
            self.dma_tx = DMA()
        self._restart_tx()
        ctrl = self._dma_pack_ctrl(
            self.dma_tx,
            size=2,
            inc_read=True,
            inc_write=False,
            treq_sel=self._pio_dreq(self.sm_dout_id, True),
            high_pri=True,
            bswap=False
        )
        self.dma_tx.config(
            read=buf,
            write=self.sm_dout,
            count=len(buf),
            ctrl=ctrl,
            trigger=True
        )
        while self.dma_tx.active():
            pass

    def unpack_words_to_i16(self, words, out_i16):
        # Debug helper for older 32-bit RX experiments.
        for i, word in enumerate(words):
            if self.rx_channel == 0:
                v = word & 0xFFFF
            else:
                v = (word >> 16) & 0xFFFF
            if v & 0x8000:
                v -= 0x10000
            out_i16[i] = v

    def pack_i16_to_words(self, in_i16, out_words):
        # Debug helper for older 32-bit TX experiments.
        if self.channel_count == 1:
            for i, sample in enumerate(in_i16):
                out_words[i] = (sample & 0xFFFF) << 16
        else:
            for i, sample in enumerate(in_i16):
                v = sample & 0xFFFF
                out_words[i] = (v << 16) | v

    def stop(self):
        if self.sm_mclk:
            self.sm_mclk.active(0)
        if self.sm_dout:
            self.sm_dout.active(0)
        if self.sm_din:
            self.sm_din.active(0)
        if self.dma_rx:
            self.dma_rx.close()
            self.dma_rx = None
        if self.dma_tx:
            self.dma_tx.close()
            self.dma_tx = None

    def start(self):
        if self.sm_mclk:
            self.sm_mclk.active(1)
        if self.sm_dout:
            self.sm_dout.active(1)
        if self.sm_din:
            self.sm_din.active(1)


class AudioLoopback:
    def __init__(self, es8311, audio_pio, sample_count=48000):
        self.es8311 = es8311
        self.audio_pio = audio_pio
        self.sample_count = sample_count

    def record_and_playback(self):
        samples = self.audio_pio.audio_in(self.sample_count)
        self.audio_pio.audio_out(samples)

    def loopback_test(self):
        self.audio_pio.mclk_pio_init()
        self.audio_pio.din_pio_init()
        self.audio_pio.dout_pio_init()

        print("Loopback test started...")
        while True:
            self.record_and_playback()
