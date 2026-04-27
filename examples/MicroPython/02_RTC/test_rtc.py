from machine import Pin, I2C
import time

# Pin definition  
I2C_SDA = 6
I2C_SDL = 7
RTC_INT = 17

# PCF85063 Driver  
class PCF85063(object):
    def __init__(self,address=0x51,i2c_num=1,i2c_sda=I2C_SDA,i2c_scl=I2C_SDL):
        self._bus = I2C(id=i2c_num,scl=Pin(i2c_scl),sda=Pin(i2c_sda),freq=100_000) # Initialize I2C
        self.address = address

        # Registar overview - crtl & status reg
        self.RTC_CTRL_1 = 0x00
        self.RTC_CTRL_2 = 0x01
        self.RTC_OFFSET = 0x02
        self.RTC_RAM_by = 0x03

        # Registar overview - time & data reg
        self.RTC_SECOND_ADDR = 0x04
        self.RTC_MINUTE_ADDR = 0x05
        self.RTC_HOUR_ADDR   = 0x06
        self.RTC_DAY_ADDR    = 0x07
        self.RTC_WDAY_ADDR   = 0x08
        self.RTC_MONTH_ADDR  = 0x09
        self.RTC_YEAR_ADDR   = 0x0a  # years 0-99; calculate real year = 1970 + RCC reg year
        
        # Registar overview - alarm reg
        self.RTC_SECOND_ALARM = 0x0b
        self.RTC_MINUTE_ALARM = 0x0c
        self.RTC_HOUR_ALARM   = 0x0d
        self.RTC_DAY_ALARM    = 0x0e
        self.RTC_WDAY_ALARM   = 0x0f

        # Registar overview - timer reg
        self.RTC_TIMER_VAL   = 0x10
        self.RTC_TIMER_MODE  = 0x11
        self.RTC_TIMER_TCF   = 0x08
        self.RTC_TIMER_TE    = 0x04
        self.RTC_TIMER_TIE   = 0x02
        self.RTC_TIMER_TI_TP = 0x01

        # Format
        self.RTC_ALARM          = 0x80  # set AEN_x registers
        self.RTC_ALARM_AIE      = 0x80  # set AIE ; enable/disable interrupt output pin
        self.RTC_ALARM_AF       = 0x40  # set AF register ; alarm flag needs to be cleared for alarm
        self.RTC_CTRL_2_DEFAULT = 0x00
        self.RTC_TIMER_FLAG     = 0x08

        self.TIMER_CLOCK_4096HZ   = 0
        self.TIMER_CLOCK_64HZ     = 1
        self.TIMER_CLOCK_1HZ      = 2
        self.TIMER_CLOCK_1PER60HZ = 3

    def decToBcd(self, val):
        return ((val // 10 * 16) + (val % 10))

    def bcdToDec(self, val):
        return ((val // 16 * 10) + (val % 16))

    def constrain(self, val, min_val, max_val):
        return min(max_val, max(min_val, val))

    def reset(self):
        self._bus.writeto_mem(self.address, self.RTC_CTRL_1, bytearray([0x58]))

    def setTime(self, hour, minute, second):
        self._bus.writeto_mem(self.address, self.RTC_SECOND_ADDR, bytearray([self.decToBcd(second)]))
        self._bus.writeto_mem(self.address, self.RTC_MINUTE_ADDR, bytearray([self.decToBcd(minute)]))
        self._bus.writeto_mem(self.address, self.RTC_HOUR_ADDR, bytearray([self.decToBcd(hour)]))

    def setDate(self, weekday, day, month, yr):
        year = yr - 1970  # convert to RTC year format 0-99
        self._bus.writeto_mem(self.address, self.RTC_DAY_ADDR, bytearray([self.decToBcd(day)]))
        self._bus.writeto_mem(self.address, self.RTC_WDAY_ADDR, bytearray([self.decToBcd(weekday)]))  # 0 for Sunday
        self._bus.writeto_mem(self.address, self.RTC_MONTH_ADDR, bytearray([self.decToBcd(month)]))
        self._bus.writeto_mem(self.address, self.RTC_YEAR_ADDR, bytearray([self.decToBcd(year)]))

    def readTime(self):
        rdata = self._bus.readfrom_mem(self.address, self.RTC_SECOND_ADDR, 7)
        data = [0] * 7
        data[0] = self.bcdToDec(rdata[0] & 0x7f) # second
        data[1] = self.bcdToDec(rdata[1] & 0x7f) # minute
        data[2] = self.bcdToDec(rdata[2] & 0x3f) # hour
        data[3] = self.bcdToDec(rdata[3] & 0x3f) # day
        data[4] = self.bcdToDec(rdata[4] & 0x07) # wday
        data[5] = self.bcdToDec(rdata[5] & 0x1f) # month
        data[6] = self.bcdToDec(rdata[6]) + 1970 # year
        
        return data
    
    def enableAlarm(self):
        control_2 = self.RTC_CTRL_2_DEFAULT | self.RTC_ALARM_AIE  # enable interrupt
        control_2 &= ~self.RTC_ALARM_AF  # clear alarm flag
        self._bus.writeto_mem(self.address, self.RTC_CTRL_2, bytearray([control_2]))

    def setAlarm(self, alarm_second, alarm_minute, alarm_hour, alarm_day, alarm_weekday):
        if alarm_second < 99:  # second
            alarm_second = self.constrain(alarm_second, 0, 59)
            alarm_second = self.decToBcd(alarm_second)
            alarm_second &= ~self.RTC_ALARM
        else:
            alarm_second = 0x0
            alarm_second |= self.RTC_ALARM

        if alarm_minute < 99:  # minute
            alarm_minute = self.constrain(alarm_minute, 0, 59)
            alarm_minute = self.decToBcd(alarm_minute)
            alarm_minute &= ~self.RTC_ALARM
        else:
            alarm_minute = 0x0
            alarm_minute |= self.RTC_ALARM

        if alarm_hour < 99:  # hour
            alarm_hour = self.constrain(alarm_hour, 0, 23)
            alarm_hour = self.decToBcd(alarm_hour)
            alarm_hour &= ~self.RTC_ALARM
        else:
            alarm_hour = 0x0
            alarm_hour |= self.RTC_ALARM

        if alarm_day < 99:  # day
            alarm_day = self.constrain(alarm_day, 1, 31)
            alarm_day = self.decToBcd(alarm_day)
            alarm_day &= ~self.RTC_ALARM
        else:
            alarm_day = 0x0
            alarm_day |= self.RTC_ALARM

        if alarm_weekday < 99:  # weekday
            alarm_weekday = self.constrain(alarm_weekday, 0, 6)
            alarm_weekday = self.decToBcd(alarm_weekday)
            alarm_weekday &= ~self.RTC_ALARM
        else:
            alarm_weekday = 0x0
            alarm_weekday |= self.RTC_ALARM

        self.enableAlarm()

        self._bus.writeto_mem(self.address, self.RTC_SECOND_ALARM, bytearray([alarm_second]))
        self._bus.writeto_mem(self.address, self.RTC_MINUTE_ALARM, bytearray([alarm_minute]))
        self._bus.writeto_mem(self.address, self.RTC_HOUR_ALARM, bytearray([alarm_hour]))
        self._bus.writeto_mem(self.address, self.RTC_DAY_ALARM, bytearray([alarm_day]))
        self._bus.writeto_mem(self.address, self.RTC_WDAY_ALARM, bytearray([alarm_weekday]))  # 0 for Sunday

    def readAlarm(self):
        rdata = self._bus.readfrom_mem(self.address, self.RTC_SECOND_ALARM, 5)
        print(rdata)

        alarm_second = rdata[0]  # read RTC_SECOND_ALARM register

        if self.RTC_ALARM & alarm_second:  # check is AEN = 1 (second alarm disabled)
            alarm_second = 99  # using 99 as code for no alarm
        else:  # else if AEN = 0 (second alarm enabled)
            alarm_second = self.bcdToDec(alarm_second & ~self.RTC_ALARM)  # remove AEN flag and convert to dec number

        alarm_minute = rdata[1]  # minute
        if self.RTC_ALARM & alarm_minute:
            alarm_minute = 99
        else:
            alarm_minute = self.bcdToDec(alarm_minute & ~self.RTC_ALARM)

        alarm_hour = rdata[2]  # hour
        if self.RTC_ALARM & alarm_hour:
            alarm_hour = 99
        else:
            alarm_hour = self.bcdToDec(alarm_hour & 0x3F)  # remove bits 7 & 6

        alarm_day = rdata[3]  # day
        if self.RTC_ALARM & alarm_day:
            alarm_day = 99
        else:
            alarm_day = self.bcdToDec(alarm_day & 0x3F)  # remove bits 7 & 6

        alarm_weekday = rdata[4]  # weekday
        if self.RTC_ALARM & alarm_weekday:
            alarm_weekday = 99
        else:
            alarm_weekday = self.bcdToDec(alarm_weekday & 0x07)  # remove bits 7,6,5,4 & 3

        print(alarm_second, alarm_minute, alarm_hour, alarm_day, alarm_weekday)

    def timerSet(self, source_clock, val, int_enable, int_pulse):
        timer_reg = [0, 0]

        # disable the countdown timer
        self.i2c.writeto_mem(self.address, self.RTC_TIMER_MODE, bytearray([0x18]))

        # clear Control_2
        self.i2c.writeto_mem(self.address, self.RTC_CTRL_2, bytearray([0x00]))

        # reconfigure timer
        timer_reg[1] |= self.RTC_TIMER_TE  # enable timer

        if int_enable:
            timer_reg[1] |= self.RTC_TIMER_TIE  # enable interrupt

        if int_pulse:
            timer_reg[1] |= self.RTC_TIMER_TI_TP  # interrupt mode

        timer_reg[1] |= source_clock << 3  # clock source

        timer_reg[0] = val

        # write timer value
        self.i2c.writeto_mem(self.address, self.RTC_TIMER_VAL, bytearray([timer_reg[0]]))
        self.i2c.writeto_mem(self.address, self.RTC_TIMER_MODE, bytearray([timer_reg[1]]))
        
# Define callback function
def pin_callback(pin):
    print(f"Interrupt detected on pin {pin}. Current value: {pin.value()}")
    data=RTC.readTime()

    print("Current time and date:")
    print(f"Second: {data[0]:02d}")
    print(f"Minute: {data[1]:02d}")
    print(f"Hour:   {data[2]:02d}")
    print(f"Day:    {data[3]:02d}")
    print(f"Weekday:{data[4]} (0=Sunday, 1=Monday, ..., 6=Saturday)")
    print(f"Month:  {data[5]:02d}")
    print(f"Year:   {data[6]}")
    
# Initialize RTC_INT to input mode and configure interrupts
pin = Pin(RTC_INT, Pin.IN, Pin.PULL_UP)  # Use internal pull-up resistor

# Configure an interrupt, triggered by both rising and falling edges.
pin.irq(trigger=Pin.IRQ_FALLING, handler=pin_callback)
    
RTC = PCF85063()
print("Init RTC done")
RTC.setDate(4,16,1,2025)
RTC.setTime(8,8,8)
RTC.setAlarm(10,8,8,16,4)
RTC.enableAlarm()

while True:
    data=RTC.readTime()
    print(f"{data[2]:02d}:{data[1]:02d}:{data[0]:02d}")
    time.sleep(1)
    pass
