import time

from machine import Pin


class BootKey:
    """
    BOOTSEL button reader.

    On RP2040/RP2350 boards the BOOT button is not a normal GPIO.
    MicroPython usually exposes it via `rp2.bootsel_button()` (or sometimes
    `machine.bootsel_button()` depending on the port/version).
    """
    def __init__(self):
        fn = None
        try:
            import rp2

            fn = rp2.bootsel_button
        except Exception:
            fn = None

        if fn is None:
            try:
                import machine

                fn = machine.bootsel_button
            except Exception:
                fn = None

        if fn is None:
            raise RuntimeError("bootsel_button not available")

        self._fn = fn

    def pressed(self):
        """
        Return True when BOOTSEL is pressed.
        """
        return bool(self._fn())


class GpioKey:
    """
    Normal GPIO button reader.

    Typical wiring uses internal pull-up and an active-low switch:
      - released: GPIO reads 1
      - pressed : GPIO reads 0
    """
    def __init__(self, pin, *, pull=Pin.PULL_UP, active_low=True):
        self._pin = Pin(pin, Pin.IN, pull)
        self._active_low = active_low

    def pressed(self):
        """
        Return True when the button is pressed.
        """
        v = self._pin.value()
        return v == 0 if self._active_low else v == 1


class Key:
    """
    A small polling-based key event generator.

    Events:
      - down / up: debounced press/release edges
      - click: single click (reported after `double_ms` timeout)
      - double: double click (reported after 2nd release + `double_ms`)
      - long: long press (reported when held for `long_ms`)
    """
    def __init__(
        self,
        name,
        read_pressed,
        on_event,
        *,
        debounce_ms=30,
        long_ms=700,
        double_ms=350,
    ):
        self.name = name
        self._read_pressed = read_pressed
        self._on_event = on_event
        self._debounce_ms = debounce_ms
        self._long_ms = long_ms
        self._double_ms = double_ms

        now = time.ticks_ms()
        raw = bool(self._read_pressed())

        # Raw (instant) state and debounced stable state.
        self._raw = raw
        self._stable = raw
        self._raw_changed_at = now

        # Press timing for long-press detection.
        self._pressed_at = None
        self._long_sent = False

        # Click aggregation for single/double click.
        self._clicks = 0
        self._click_deadline = None

    def _emit(self, event, value=None):
        self._on_event(self.name, event, value)

    def poll(self, now=None):
        # Call this periodically (e.g. every 5~20ms).
        if now is None:
            now = time.ticks_ms()

        # 1) Read the raw hardware state.
        raw = bool(self._read_pressed())
        if raw != self._raw:
            self._raw = raw
            self._raw_changed_at = now

        # 2) Debounce: accept a state change only if it stays unchanged for `debounce_ms`.
        if raw != self._stable:
            if time.ticks_diff(now, self._raw_changed_at) >= self._debounce_ms:
                self._stable = raw
                if raw:
                    self._pressed_at = now
                    self._long_sent = False
                    self._emit("down")
                else:
                    self._emit("up")
                    if self._pressed_at is not None:
                        held = time.ticks_diff(now, self._pressed_at)
                        if held >= self._long_ms:
                            if not self._long_sent:
                                self._emit("long")
                        else:
                            # Defer click reporting until we know whether a 2nd click follows.
                            self._clicks += 1
                            self._click_deadline = time.ticks_add(now, self._double_ms)
                    self._pressed_at = None

        # 3) Long-press: fire once when held long enough (without waiting for release).
        if self._stable and self._pressed_at is not None and not self._long_sent:
            if time.ticks_diff(now, self._pressed_at) >= self._long_ms:
                self._long_sent = True
                self._clicks = 0
                self._click_deadline = None
                self._emit("long")

        # 4) Click/double: when timeout expires, convert pending click count into an event.
        if self._click_deadline is not None:
            if time.ticks_diff(now, self._click_deadline) >= 0:
                clicks = self._clicks
                self._clicks = 0
                self._click_deadline = None
                if clicks == 1:
                    self._emit("click")
                elif clicks == 2:
                    self._emit("double")
                else:
                    self._emit("multi", clicks)


def _on_key_event(name, event, value):
    if value is None:
        print(name, event)
    else:
        print(name, event, value)


def main(*, banner=False, poll_ms=10):
    if banner:
        print("Key demo started.")
        print("Try: single click / double click / long press.")
        print("BOOT  : BOOTSEL button (special pin).")
        print("POWER : GPIO24 button (pull-up, active-low).")
        print("Stop  : Ctrl+C in REPL.")

    # BOOTSEL: special button (not a normal GPIO).
    boot_key = Key("boot", BootKey().pressed, _on_key_event)
    # POWER: normal GPIO key on GPIO24 (active-low with pull-up).
    power_key = Key("power", GpioKey(24, pull=Pin.PULL_UP, active_low=True).pressed, _on_key_event)

    while True:
        now = time.ticks_ms()
        boot_key.poll(now)
        power_key.poll(now)
        # Polling interval: trade-off between CPU usage and responsiveness.
        time.sleep_ms(poll_ms)


if __name__ == "__main__":
    main(banner=True)

