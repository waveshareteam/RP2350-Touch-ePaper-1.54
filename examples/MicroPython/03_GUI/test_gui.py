# *****************************************************************************
# * | File        :   Pico_ePaper-1.54.py
# * | Author      :   Waveshare team
# * | Function    :   Electronic paper driver (1.54inch V2)
# * | Info        :
# *----------------
# * | This version:   V1.0
# * | Date        :   2025-10-20
# # | Info        :   python demo
# -----------------------------------------------------------------------------
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

from machine import Pin, SPI
import framebuf
import utime

lut_full_update= [
    0x80, 0x48, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x40, 0x48, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x80, 0x48, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x40, 0x48, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x8, 0x1, 0x0, 0x8, 0x1, 0x0, 0x2,
    0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0,
    0x22, 0x17, 0x41, 0x0, 0x32, 0x20
]

lut_partial_update = [
0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x80,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x40,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0xF,0x0,0x0,0x0,0x0,0x0,0x0,
0x1,0x1,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x22,0x22,0x22,0x22,0x22,0x22,0x0,0x0,0x0,
0x02,0x17,0x41,0xB0,0x32,0x28,
]

EPD_WIDTH       = 200
EPD_HEIGHT      = 200

# SPI Configuration
SPI_BUS = 1                  # SPI bus number to use
SPI_FREQ = 4000_000          # SPI communication frequency
SPI_POLARITY = 0             # SPI clock polarity
SPI_PHASE = 0                # SPI clock phase
SPI_SCK_PIN = 10             # SPI clock pin
SPI_MOSI_PIN = 11            # SPI data output pin
SPI_MISO_PIN = None          # SPI data input pin (not needed for LCD)

RST_PIN         = 14
DC_PIN          = 12
CS_PIN          = 9
BUSY_PIN        = 15
PWR_PIN         = 13

FULL_UPDATE = 0
PART_UPDATE = 1

class EPD_1in54(framebuf.FrameBuffer):
    def __init__(self):
        self.pwr_pin = Pin(PWR_PIN, Pin.OUT)
        self.digital_write(self.pwr_pin, 0)
        
        self.reset_pin = Pin(RST_PIN, Pin.OUT)
        self.busy_pin = Pin(BUSY_PIN, Pin.IN, Pin.PULL_UP)
        self.cs_pin = Pin(CS_PIN, Pin.OUT)
        self.width = EPD_WIDTH
        self.height = EPD_HEIGHT
        
        self.full_lut = lut_full_update
        self.partial_lut = lut_partial_update
        
        self.full_update = FULL_UPDATE
        self.part_update = PART_UPDATE
        
        self.spi = SPI(SPI_BUS)
        self.spi.init(baudrate=SPI_FREQ)
        self.spi = SPI(SPI_BUS,SPI_FREQ,polarity=SPI_POLARITY, phase=SPI_PHASE,sck=Pin(SPI_SCK_PIN),mosi=Pin(SPI_MOSI_PIN),miso=SPI_MISO_PIN)
        self.dc_pin = Pin(DC_PIN, Pin.OUT)
        
        
        self.buffer = bytearray(self.height * self.width // 8)
        super().__init__(self.buffer, self.width, self.height, framebuf.MONO_HLSB)
        self.init(FULL_UPDATE)

    def digital_write(self, pin, value):
        pin.value(value)

    def digital_read(self, pin):
        return pin.value()

    def delay_ms(self, delaytime):
        utime.sleep(delaytime / 1000.0)

    def spi_writebyte(self, data):
        self.spi.write(bytearray(data))

    def module_exit(self):
        self.digital_write(self.reset_pin, 0)
        self.digital_write(self.pwr_pin, 1)

    # Hardware reset
    def reset(self):
        self.digital_write(self.reset_pin, 1)
        self.delay_ms(50)
        self.digital_write(self.reset_pin, 0)
        self.delay_ms(2)
        self.digital_write(self.reset_pin, 1)
        self.delay_ms(50)   

    def send_command(self, command):
        self.digital_write(self.dc_pin, 0)
        self.digital_write(self.cs_pin, 0)
        self.spi_writebyte([command])
        self.digital_write(self.cs_pin, 1)

    def send_data(self, data):
        self.digital_write(self.dc_pin, 1)
        self.digital_write(self.cs_pin, 0)
        self.spi_writebyte([data])
        self.digital_write(self.cs_pin, 1)
        
    def send_data1(self, buf):
        self.digital_write(self.dc_pin, 1)
        self.digital_write(self.cs_pin, 0)
        self.spi.write(bytearray(buf))
        self.digital_write(self.cs_pin, 1)
        
    def ReadBusy(self):
        print('busy')
        while(self.digital_read(self.busy_pin) == 1):      # 0: idle, 1: busy
            self.delay_ms(10)    
        print('busy release')
    
    # Set LUT (sync with C code EPD_1IN54_V2_SetLut)
    def set_lut(self, lut):
        self.send_command(0x32)
        for i in range(153):
            self.send_data(lut[i])
        self.ReadBusy()
        
        self.send_command(0x3f)
        self.send_data(lut[153])
        
        self.send_command(0x03)
        self.send_data(lut[154])
        
        self.send_command(0x04)
        self.send_data(lut[155])
        self.send_data(lut[156])
        self.send_data(lut[157])
        
        self.send_command(0x2c)
        self.send_data(lut[158])
    
    # Set windows (sync with C code EPD_1IN54_V2_SetWindows)
    def set_windows(self, x_start, y_start, x_end, y_end):
        self.send_command(0x44)  # SET_RAM_X_ADDRESS_START_END_POSITION
        self.send_data((x_start >> 3) & 0xFF)
        self.send_data((x_end >> 3) & 0xFF)
        
        self.send_command(0x45)  # SET_RAM_Y_ADDRESS_START_END_POSITION
        self.send_data(y_start & 0xFF)
        self.send_data((y_start >> 8) & 0xFF)
        self.send_data(y_end & 0xFF)
        self.send_data((y_end >> 8) & 0xFF)
    
    # Set cursor (sync with C code EPD_1IN54_V2_SetCursor)
    def set_cursor(self, x, y):
        self.send_command(0x4E)  # SET_RAM_X_ADDRESS_COUNTER
        self.send_data(x & 0xFF)
        
        self.send_command(0x4F)  # SET_RAM_Y_ADDRESS_COUNTER
        self.send_data(y & 0xFF)
        self.send_data((y >> 8) & 0xFF)

        
    def TurnOnDisplay(self):
        self.send_command(0x22)
        self.send_data(0xC7)
        self.send_command(0x20)        
        self.ReadBusy()

    def TurnOnDisplayPart(self):
        self.send_command(0x22)
        self.send_data(0xcf)
        self.send_command(0x20)        
        self.ReadBusy()

    def init(self, update):
        print('Initializing e-Paper...')
        self.reset()
        if update == self.full_update:
            self.ReadBusy()
            self.send_command(0x12)  # SWRESET
            self.ReadBusy()
            
            self.send_command(0x01)  # Driver output control
            self.send_data(0xC7)
            self.send_data(0x00)
            self.send_data(0x01)
            
            self.send_command(0x11)  # Data entry mode
            self.send_data(0x01)
            
            # Set windows
            self.set_windows(0, self.height - 1, self.width - 1, 0)
            
            self.send_command(0x3C)  # Border waveform
            self.send_data(0x01)
            
            self.send_command(0x18)
            self.send_data(0x80)
            
            self.send_command(0x22)  # Load Temperature and waveform setting
            self.send_data(0xB1)
            self.send_command(0x20)
            
            # Set cursor
            self.set_cursor(0, self.height - 1)
            
            # Set LUT
            self.set_lut(self.full_lut)
            self.ReadBusy()
        else:
            # Partial update initialization
            self.set_lut(self.partial_lut)
            
            self.send_command(0x37)
            self.send_data(0x00)
            self.send_data(0x00)
            self.send_data(0x00)
            self.send_data(0x00)
            self.send_data(0x00)
            self.send_data(0x40)
            self.send_data(0x00)
            self.send_data(0x00)
            self.send_data(0x00)
            self.send_data(0x00)
            
            self.send_command(0x3C)  # Border waveform
            self.send_data(0x80)
            
            self.send_command(0x22)
            self.send_data(0xC0)
            self.send_command(0x20)
            self.ReadBusy()
        return 0       
 
    def display(self, image):
        self.send_command(0x24)
        self.send_data1(image)
        self.TurnOnDisplay()
        
    def displayPartial(self, image):
        self.send_command(0x24)
        self.send_data1(image)
        self.TurnOnDisplayPart()

    def displayPartBaseImage(self, image):
        self.send_command(0x24)
        self.send_data1(image) 
                
        self.send_command(0x26)
        self.send_data1(image)
        
        self.TurnOnDisplay()
    
    def Clear(self, color):
        self.send_command(0x24)
        self.send_data1([color] * self.height * int(self.width / 8))
        
        self.send_command(0x26)
        self.send_data1([color] * self.height * int(self.width / 8))
        
        self.TurnOnDisplay()

    def sleep(self):
        self.send_command(0x10)
        self.send_data(0x01)
        self.delay_ms(100)
        self.module_exit()
        
        
if __name__=='__main__':
    epd = EPD_1in54()
    epd.Clear(0xff)
    
    epd.fill(0xff)
    epd.text("Waveshare", 0, 10, 0x00)
    epd.text("RP2350-Touch-ePaper-1.54", 0, 30, 0x00)
    epd.text("Raspberry Pico", 0, 50, 0x00)
    epd.text("Hello World", 0, 70, 0x00)
    epd.display(epd.buffer)
    epd.delay_ms(2000)
    
    epd.vline(10, 90, 60, 0x00)
    epd.vline(90, 90, 60, 0x00)
    epd.hline(10, 90, 80, 0x00)
    epd.hline(10, 150, 80, 0x00)
    epd.line(10, 90, 90, 150, 0x00)
    epd.line(90, 90, 10, 150, 0x00)
    epd.display(epd.buffer)
    epd.delay_ms(2000)
    
    epd.rect(95, 100, 50, 40, 0x00)
    epd.fill_rect(145, 100, 50, 40, 0x00)
    epd.displayPartBaseImage(epd.buffer)
    epd.delay_ms(2000)
    
    epd.init(epd.part_update)
    for i in range(0, 10):
        epd.fill_rect(20, 160, 60, 20, 0xFF)
        epd.text(f"Count: {i}", 15, 160, 0x00)
        epd.displayPartial(epd.buffer)
        epd.delay_ms(500)
        
    epd.init(epd.full_update)
    epd.Clear(0xff)
    epd.delay_ms(2000)
    epd.sleep()