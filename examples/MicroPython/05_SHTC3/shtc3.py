from machine import Pin, I2C
import time

# SHTC3 driver + example in a single file.
# - Import this file in your project: from shtc3 import SHTC3
# - Or run it directly to see the example output (main() at the bottom).

# Default hardware configuration (you can override these via SHTC3(...)).
SHTC3_I2C_NUM = 1
SHTC3_I2C_SCL = 7
SHTC3_I2C_SDA = 6
SHTC3_I2C_ADDR = 0x70
SHTC3_I2C_FREQ = 100_000

# SHTC3 command set (16-bit commands, sent MSB first).
SHTC3_REG_SLEEP = 0xB098
SHTC3_REG_WAKEUP = 0x3517
SHTC3_REG_SOFTRESET = 0x805D
SHTC3_REG_READID = 0xEFC8

SHTC3_REG_NORMAL_T_F = 0x7866
SHTC3_REG_NORMAL_H_F = 0x58E0

SHTC3_REG_NORMAL_T_F_STRETCH = 0x7CA2
SHTC3_REG_NORMAL_H_F_STRETCH = 0x5C24

SHTC3_REG_LOWPOWER_T_F = 0x609C
SHTC3_REG_LOWPOWER_H_F = 0x401A

SHTC3_REG_LOWPOWER_T_F_STRETCH = 0x6458
SHTC3_REG_LOWPOWER_H_F_STRETCH = 0x44DE

SHTC3_NORMAL_MEAS = [SHTC3_REG_NORMAL_T_F, SHTC3_REG_NORMAL_H_F]
SHTC3_NORMAL_MEAS_STRETCH = [SHTC3_REG_NORMAL_T_F_STRETCH, SHTC3_REG_NORMAL_H_F_STRETCH]
SHTC3_LOWPOWER_MEAS = [SHTC3_REG_LOWPOWER_T_F, SHTC3_REG_LOWPOWER_H_F]
SHTC3_LOWPOWER_MEAS_STRETCH = [SHTC3_REG_LOWPOWER_T_F_STRETCH, SHTC3_REG_LOWPOWER_H_F_STRETCH]

SHTC3_MEAS = [SHTC3_NORMAL_MEAS, SHTC3_LOWPOWER_MEAS]
SHTC3_MEAS_STRETCH = [SHTC3_NORMAL_MEAS_STRETCH, SHTC3_LOWPOWER_MEAS_STRETCH]
SHTC3_MEAS_ALL = [SHTC3_MEAS, SHTC3_MEAS_STRETCH]

# Internal sentinel used to detect "parameter not provided".
_DEFAULT = object()


class SHTC3(object):
    # Minimal SHTC3 driver for MicroPython.
    # The class keeps small buffers to avoid extra allocations during sampling.
    def __init__(
        self,
        i2c_num=SHTC3_I2C_NUM,
        i2c_scl=SHTC3_I2C_SCL,
        i2c_sda=SHTC3_I2C_SDA,
        address=SHTC3_I2C_ADDR,
        i2c=None,
        crc_fail_return=(None, None),
    ):
        # crc_fail_return controls what you get when CRC check fails.
        # For example: SHTC3(crc_fail_return=(0, 0)) to mimic some older examples.
        self._address = address
        self._crc_fail_return = crc_fail_return
        self._cmd = bytearray(2)
        self._buffer6 = bytearray(6)
        self._buffer3 = bytearray(3)
        self._mv6 = memoryview(self._buffer6)
        # If an I2C object is provided, reuse it; otherwise create a new I2C bus.
        self.i2c = i2c or I2C(
            id=i2c_num,
            scl=Pin(i2c_scl, pull=Pin.PULL_UP),
            sda=Pin(i2c_sda, pull=Pin.PULL_UP),
            freq=SHTC3_I2C_FREQ,
        )

        # Bring the sensor into a known state.
        self.wakeup()
        self.soft_reset()
        time.sleep_ms(1)

    @staticmethod
    def crc8(buffer) -> int:
        # CRC-8 with polynomial 0x31, init 0xFF (SHTC3 datasheet).
        crc = 0xFF
        for byte in buffer:
            crc ^= byte
            for _ in range(8):
                if crc & 0x80:
                    crc = ((crc << 1) ^ 0x31) & 0xFF
                else:
                    crc = (crc << 1) & 0xFF
        return crc

    def _write_command(self, command: int):
        # Send a 16-bit command (MSB first).
        self._cmd[0] = command >> 8
        self._cmd[1] = command & 0xFF
        self.i2c.writeto(self._address, self._cmd)

    def sleep(self):
        # Put the sensor into sleep mode to save power.
        self._write_command(SHTC3_REG_SLEEP)
        time.sleep_us(300)

    def wakeup(self):
        # Wake up the sensor before accessing it.
        self._write_command(SHTC3_REG_WAKEUP)
        time.sleep_us(300)

    def soft_reset(self):
        # Soft reset: recommended after power-up / wake-up.
        self._write_command(SHTC3_REG_SOFTRESET)
        time.sleep_us(300)

    def read_id(self):
        # Read device ID register and verify CRC.
        self._write_command(SHTC3_REG_READID)
        self.i2c.readfrom_into(self._address, self._buffer3)
        if self._buffer3[2] != self.crc8(self._buffer3[0:2]):
            return None
        return (self._buffer3[0] << 8) | self._buffer3[1]

    def measurement(
        self,
        hum_first=False,
        low_power_meas=False,
        stretch=False,
        crc_fail_return=_DEFAULT,
        hum_frist=None,
    ):
        # Take one measurement and return (temperature_c, humidity_rh).
        # hum_frist is kept for backward compatibility (typo in some older examples).
        if hum_frist is not None:
            hum_first = hum_frist

        if crc_fail_return is _DEFAULT:
            crc_fail_return = self._crc_fail_return

        command = SHTC3_MEAS_ALL[stretch][low_power_meas][hum_first]
        self._write_command(command)
        # Wait for conversion time (datasheet typical values).
        if low_power_meas:
            time.sleep_ms(2)
        else:
            time.sleep_ms(14)

        # Read 6 bytes: T(2)+CRC(1) and RH(2)+CRC(1) in one transaction.
        self.i2c.readfrom_into(self._address, self._buffer6)

        if hum_first:
            hum_off = 0
            temp_off = 3
        else:
            temp_off = 0
            hum_off = 3

        mv = self._mv6
        temp_crc_ok = mv[temp_off + 2] == self.crc8(mv[temp_off : temp_off + 2])
        hum_crc_ok = mv[hum_off + 2] == self.crc8(mv[hum_off : hum_off + 2])
        if not (temp_crc_ok and hum_crc_ok):
            return crc_fail_return

        t_raw = (mv[temp_off] << 8) | mv[temp_off + 1]
        rh_raw = (mv[hum_off] << 8) | mv[hum_off + 1]

        # Convert raw values into human-readable units (from datasheet).
        t = -45.0 + 175.0 * t_raw / 65536.0
        rh = 100.0 * rh_raw / 65536.0
        return (t, rh)

    def read(self, low_power=False, stretch=False, crc_fail_return=_DEFAULT):
        # Friendly alias: read temperature first (same as measurement(hum_first=False, ...)).
        return self.measurement(
            hum_first=False,
            low_power_meas=low_power,
            stretch=stretch,
            crc_fail_return=crc_fail_return,
        )


def main(
    i2c=None,
    low_power=False,
    stretch=False,
    interval_s=1,
    crc_fail_return=_DEFAULT,
):
    # Example loop: print values once per interval_s seconds.
    shtc3 = SHTC3(i2c=i2c)
    try:
        sensor_id = shtc3.read_id()
        if sensor_id is None:
            print("SHTC3 ID read failed (CRC mismatch)")
        else:
            print("SHTC3 ID: 0x{:04x}".format(sensor_id))

        print("Start monitoring temperature and humidity...")
        print("-" * 40)

        while True:
            t, rh = shtc3.read(
                low_power=low_power,
                stretch=stretch,
                crc_fail_return=crc_fail_return,
            )
            if t is None or rh is None:
                print("CRC mismatch, skip this sample")
            else:
                print("Temperature: {:.2f} °C, Humidity: {:.2f} %".format(t, rh))
            time.sleep(interval_s)
    finally:
        shtc3.sleep()


if __name__ == "__main__":
    try:
        # Run the built-in example.
        main()
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user.")
