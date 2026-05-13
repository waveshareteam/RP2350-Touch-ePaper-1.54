import machine
import time

# FT6336U register definitions
FT6336U_ADDR = 0x38
FT6336U_ADDR_CHIP_ID       = 0xA3
FT6336U_GESTURE_ENABLE     = 0x01
FT6336U_ADDR_GESTURE_EN    = 0xD0
FT6336U_ADDR_GESTURE_OUTPUT= 0xD3
FT6336U_ADDR_TD_STATUS     = 0x02
FT6336U_ADDR_TOUCH1_X      = 0x03
FT6336U_ADDR_TOUCH1_Y      = 0x05

# Mode definitions
FT6336U_Point_Mode   = 0
FT6336U_Gesture_Mode = 1

# GPIO Pin definitions for RP2350 Touch ePaper
EPD_TP_I2C_NUM = 1
EPD_TP_SDA_PIN = 6
EPD_TP_SCL_PIN = 7
EPD_TP_INT_PIN = 8
EPD_TP_RST_PIN = 16

class touch_ft6336u:
    def __init__(self, device_addr=FT6336U_ADDR, mode=FT6336U_Point_Mode,
                 i2c_num=EPD_TP_I2C_NUM, i2c_sda=EPD_TP_SDA_PIN, 
                 i2c_scl=EPD_TP_SCL_PIN, irq_pin=EPD_TP_INT_PIN, 
                 rst_pin=EPD_TP_RST_PIN):

        # I2C setup - Use software I2C for flexible pin assignment
        self.sda_pin = machine.Pin(i2c_sda)
        self.scl_pin = machine.Pin(i2c_scl)
        self.bus = machine.I2C(id=i2c_num,
                               scl=self.scl_pin,
                               sda=self.sda_pin,
                               freq=400_000)

        self.device_addr = device_addr
        self.mode = mode

        # Interrupt pin
        self.int = machine.Pin(irq_pin, machine.Pin.IN, machine.Pin.PULL_UP)

        # Reset pin
        self.rst = machine.Pin(rst_pin, machine.Pin.OUT)

        # Touch info (only one point supported)
        self.point_count = 0
        self.point = {"x": 0, "y": 0}

        # Initialize
        self.reset()
        self.init_chip()

        # IRQ callback
        self.int.irq(handler=self.int_cb, trigger=machine.Pin.IRQ_FALLING)

    # ----------------------------------
    # Reset FT6336U
    # ----------------------------------
    def reset(self):
        self.rst(1)
        time.sleep_ms(200)
        self.rst(0)
        time.sleep_ms(200)
        self.rst(1)
        time.sleep_ms(200)

    # ----------------------------------
    # Write register
    # ----------------------------------
    def write_reg(self, reg, value):
        try:
            self.bus.writeto(self.device_addr, bytes([reg, value]))
        except Exception as e:
            print("I2C write error:", e)

    # ----------------------------------
    # Read bytes
    # ----------------------------------
    def read_bytes(self, reg, length):
        try:
            self.bus.writeto(self.device_addr, bytes([reg]))
            return self.bus.readfrom(self.device_addr, length)
        except Exception as e:
            print("I2C read error:", e)
            return None

    # ----------------------------------
    # Initialize FT6336U
    # ----------------------------------
    def init_chip(self):
        if self.mode == FT6336U_Gesture_Mode:
            self.write_reg(FT6336U_ADDR_GESTURE_EN, FT6336U_GESTURE_ENABLE)
        else:
            self.write_reg(FT6336U_ADDR_GESTURE_EN, 0x00)

        chip_id = self.read_id()
        print("FT6336U ID =", hex(chip_id))

        if chip_id != 0x64:
            print("FT6336U ID error!")
        else:
            print("FT6336U init ok!")

    # ----------------------------------
    # Read chip ID
    # ----------------------------------
    def read_id(self):
        buf = self.read_bytes(FT6336U_ADDR_CHIP_ID, 1)
        if buf is not None:
            return buf[0]
        return 0

    # ----------------------------------
    # IRQ callback
    # ----------------------------------
    def int_cb(self, pin):
        self.read_touch_data()

    # ----------------------------------
    # Read touch point data (single point)
    # ----------------------------------
    def read_touch_data(self):
        buf = self.read_bytes(FT6336U_ADDR_TD_STATUS, 1)
        if buf is None:
            return

        self.point_count = buf[0] & 0x0F

        if self.point_count == 0:
            return

        # Read touch1 (x, y)
        xy = self.read_bytes(FT6336U_ADDR_TOUCH1_X, 4)
        if xy:
            self.point["x"] = ((xy[0] & 0x0F) << 8) | xy[1]
            self.point["y"] = ((xy[2] & 0x0F) << 8) | xy[3]

    # ----------------------------------
    # Return touch point (single touch)
    # ----------------------------------
    def get_touch_xy(self):
        if self.point_count > 0:
            self.point_count = 0
            return [self.point]  # always return list for compatibility
        return None

    # ----------------------------------
    # Get gesture
    # ----------------------------------
    def get_gesture(self):
        buf = self.read_bytes(FT6336U_ADDR_GESTURE_OUTPUT, 1)
        if buf is not None:
            return buf[0]
        return 0
