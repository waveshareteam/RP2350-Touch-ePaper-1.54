from machine import I2C, Pin
import time
from array import array
import es8311
import audio_pio_mpy

ES8311_SDA_PIN = 6
ES8311_SCL_PIN = 7
PA_CTRL_PIN = 0

# User-tunable audio settings.
SAMPLE_FREQ = 24000
MCLK_FREQ = SAMPLE_FREQ * 256

# Loopback clip length in milliseconds. This allocates
# SAMPLE_FREQ * RECORD_DURATION_MS / 1000 int16 samples, so keep it modest.
# 2000 ms works well on RP2350; values above about 3000 ms may exhaust heap.
RECORD_DURATION_MS = 2000
MAX_RECOMMENDED_RECORD_DURATION_MS = 3000

# ES8311 DAC volume is dB-based, not a linear percentage.
# Keep 80-100 for this loopback demo unless using external amplification.
DAC_VOLUME = 90
MIC_GAIN = 0
MIC_CONDITION_SHIFT = 2
CLIP_THRESHOLD = 32000

# Set this to False only when testing a MicroPython build without rp2.DMA.
USE_DMA = True

# Optional debug helpers. Keep disabled for normal loopback use.
DEBUG_AUDIO = False
ENABLE_DMA_TONE_TEST = False
DMA_TONE_HZ = 1000
DMA_TONE_DURATION_MS = 800
DMA_TONE_AMPLITUDE = 12000

# PIO state machine IDs. Values >= 4 select PIO1 on MicroPython rp2.
SM_DOUT_ID = 0
SM_DIN_ID = 5
SM_MCLK_ID = 2

def init_hardware():
    Pin(PA_CTRL_PIN, Pin.OUT, value=1)
    i2c = I2C(1, scl=Pin(ES8311_SCL_PIN), sda=Pin(ES8311_SDA_PIN), freq=400000)
    print(f"I2C devices: {i2c.scan()}")
    return i2c

def init_es8311(i2c):
    codec = es8311.ES8311(i2c)
    chip_id = codec.read_id()
    print(f"ES8311 Chip ID: 0x{chip_id:04x}")

    codec.init(
        mclk_freq=MCLK_FREQ,
        sample_freq=SAMPLE_FREQ,
        res_in=16,
        res_out=16,
        volume=DAC_VOLUME,
        mic_gain=MIC_GAIN
    )
    codec.mute(False)
    print("ES8311 initialized successfully")
    return codec

def init_audio_pio():
    audio_pio = audio_pio_mpy.AudioPIO(
        mclk_pin=3,
        dout_pin=1,
        din_pin=2,
        lrclk_pin=5,
        bclk_pin=4,
        sm_dout_id=SM_DOUT_ID,
        sm_din_id=SM_DIN_ID,
        sm_mclk_id=SM_MCLK_ID
    )
    audio_pio.mclk_freq = MCLK_FREQ
    audio_pio.sample_freq = SAMPLE_FREQ
    audio_pio.channel_count = 2
    audio_pio.rx_channel = 0
    return audio_pio

def check_pin_toggles(pin_num, samples=20000):
    p = Pin(pin_num, Pin.IN)
    last = p.value()
    changes = 0
    for _ in range(samples):
        v = p.value()
        if v != last:
            changes += 1
            last = v
    return changes

def check_pin_level_stats(pin_num, samples=20000):
    p = Pin(pin_num, Pin.IN)
    high = 0
    low = 0
    changes = 0
    last = p.value()
    for _ in range(samples):
        v = p.value()
        if v:
            high += 1
        else:
            low += 1
        if v != last:
            changes += 1
            last = v
    return low, high, changes

def dump_codec_audio_regs(codec):
    regs = (0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0D, 0x0E, 0x12, 0x13, 0x14,
            0x16, 0x17, 0x1C, 0x31, 0x32, 0x37)
    print("ES8311 audio regs:")
    for reg in regs:
        print("  REG 0x%02X = 0x%02X" % (reg, codec.read_reg(reg)))

def fill_tone(buf, hz=1000, amp=12000):
    half_period = max(1, int(SAMPLE_FREQ / (hz * 2)))
    v = amp
    c = 0
    for i in range(len(buf)):
        buf[i] = v
        c += 1
        if c >= half_period:
            c = 0
            v = -v

def condition_mic_samples(buf, gain_shift=MIC_CONDITION_SHIFT):
    # Remove DC offset and attenuate the microphone signal before playback.
    total = 0
    for v in buf:
        total += v
    dc = total // len(buf)

    for i in range(len(buf)):
        v = (buf[i] - dc) >> gain_shift
        if v > 32767:
            v = 32767
        elif v < -32768:
            v = -32768
        buf[i] = v

def count_clipped_samples(buf, threshold=CLIP_THRESHOLD):
    clipped = 0
    for v in buf:
        if v >= threshold or v <= -threshold:
            clipped += 1
    return clipped

def main():
    print("=" * 50)
    print("ES8311 Audio Test - MicroPython")
    print("=" * 50)

    i2c = init_hardware()
    codec = init_es8311(i2c)
    audio_pio = init_audio_pio()

    audio_pio.mclk_pio_init()
    audio_pio.dout_pio_init()

    if DEBUG_AUDIO:
        bclk_changes = check_pin_toggles(4)
        lrclk_changes = check_pin_toggles(5)
        din_low, din_high, din_changes = check_pin_level_stats(2)
        print(f"\nClock check: BCLK changes={bclk_changes}, LRCLK changes={lrclk_changes}")
        print(f"DIN GPIO2 check before RX PIO: low={din_low}, high={din_high}, changes={din_changes}")
        dump_codec_audio_regs(codec)

    if RECORD_DURATION_MS > MAX_RECOMMENDED_RECORD_DURATION_MS:
        print("Warning: RECORD_DURATION_MS is high and may exhaust MicroPython heap.")

    samples_per_cycle = int(SAMPLE_FREQ * RECORD_DURATION_MS / 1000)
    samples_buf = array("h", [0] * samples_per_cycle)
    tone_samples = int(SAMPLE_FREQ * DMA_TONE_DURATION_MS / 1000)
    tone_buf_h = array("h", [0] * tone_samples)
    pio_play_buf32 = None
    if not (USE_DMA and audio_pio.dma_supported()):
        pio_play_buf32 = array("I", [0] * samples_per_cycle)

    print(f"\nConfiguration:")
    print(f"  Sample Rate: {SAMPLE_FREQ} Hz")
    print(f"  MCLK Frequency: {MCLK_FREQ} Hz")
    print(f"  Record Duration: {RECORD_DURATION_MS} ms")
    print(f"  Samples per Cycle: {samples_per_cycle}")
    print(f"  DMA Available: {audio_pio.dma_supported()}")
    print(f"  DMA Enabled: {USE_DMA and audio_pio.dma_supported()}")
    print(f"\nStarting loopback test... (Press Ctrl+C to stop)")
    print("-" * 50)

    try:
        # Enable this block when checking the speaker path without the microphone.
        if ENABLE_DMA_TONE_TEST and USE_DMA and audio_pio.dma_supported():
            print(f"\nDMA tone test: {DMA_TONE_HZ}Hz")
            fill_tone(tone_buf_h, hz=DMA_TONE_HZ, amp=DMA_TONE_AMPLITUDE)
            audio_pio.dma_play_from_i16(tone_buf_h)

        audio_pio.din_pio_init()

        cycle_count = 0
        while True:
            cycle_count += 1
            print(f"\n[Cycle {cycle_count}] Starting recording...")

            record_start = time.ticks_ms()
            if USE_DMA and audio_pio.dma_supported():
                audio_pio.dma_record_into(samples_buf)
            else:
                audio_pio.audio_in_into(samples_buf)
            record_time = time.ticks_diff(time.ticks_ms(), record_start)
            print(f"[Cycle {cycle_count}] Recording complete: {len(samples_buf)} samples in {record_time}ms")
            mn = min(samples_buf)
            mx = max(samples_buf)
            clipped = count_clipped_samples(samples_buf)
            print(f"[Cycle {cycle_count}] Recorded min={mn} max={mx} clipped={clipped}")
            condition_mic_samples(samples_buf)
            mn = min(samples_buf)
            mx = max(samples_buf)
            print(f"[Cycle {cycle_count}] Conditioned min={mn} max={mx}")

            print(f"[Cycle {cycle_count}] Starting playback...")
            play_start = time.ticks_ms()
            if USE_DMA and audio_pio.dma_supported():
                audio_pio.dma_play_from_i16(samples_buf)
            else:
                audio_pio.audio_out_i16(samples_buf, pio_play_buf32)
            play_time = time.ticks_diff(time.ticks_ms(), play_start)
            print(f"[Cycle {cycle_count}] Playback complete in {play_time}ms")

            print(f"[Cycle {cycle_count}] Next cycle...")
            time.sleep_ms(200)
    except KeyboardInterrupt:
        print("\n\nStopping audio test...")
        audio_pio.stop()
        print("Audio PIO stopped")

if __name__ == "__main__":
    main()
