/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
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
******************************************************************************/
#include "DEV_Config.h"

uint slice_num;


/**
 * stdio
 **/
void DEV_Stdio_Init(void)
{
    Serial.begin(115200);

    while (!Serial) {
        delay(10);
    }
}

void DEV_Stdio_deInit(void)
{
}

/**
 * delay
 **/
void DEV_Delay_ms(uint32_t xms)
{
    delay(xms);
}

void DEV_Delay_us(uint32_t xus)
{
    delayMicroseconds(xus);
}

/**
 * GPIO
 **/
void DEV_GPIO_Init(void)
{

    DEV_GPIO_Mode(LCD_RST_PIN, 1);
    DEV_GPIO_Mode(LCD_DC_PIN, 1);
    DEV_GPIO_Mode(LCD_CS_PIN, 1);
    DEV_GPIO_Mode(LCD_BUSY_PIN, 0);

    DEV_Digital_Write(LCD_CS_PIN, 1);
    DEV_Digital_Write(LCD_DC_PIN, 0);

    DEV_LCD_Power_GPIO_Init();
}

void DEV_GPIO_deInit(void)
{
}

void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode)
{
    if (Mode == 0 || Mode == GPIO_IN)
    {
        pinMode(Pin, INPUT);
    }
    else
    {
        pinMode(Pin, OUTPUT);
    }
}

void DEV_Digital_Write(uint16_t Pin, uint8_t Value)
{
    digitalWrite(Pin, Value);
}

uint8_t DEV_Digital_Read(uint16_t Pin)
{
    return digitalRead(Pin);
}


void DEV_LED_Init(void)
{
    DEV_GPIO_Mode(LED_GREEN_PIN, 1);
}

void DEV_LED_On(void)
{
    DEV_Digital_Write(LED_GREEN_PIN, 0);
}

void DEV_LED_Off(void)
{
    DEV_Digital_Write(LED_GREEN_PIN, 1);
}

/**
 * ADC
 **/
void DEV_ADC_Init(void)
{
}

void DEV_ADC_deInit(void)
{

}

uint16_t DEV_ADC_Read(void)
{
    return 0;
}

/**
 * PWM
 **/
void DEV_PWM_Init(void)
{
}

void DEV_PWM_deInit(void)
{
}

void DEV_SET_PWM(uint8_t Value)
{
    if (Value < 0 || Value > 100)
    {
        printf("DEV_SET_PWM Error \r\n");
    }
    else
    {
        analogWrite(LCD_BL_PIN, Value * 2.55);
    }
}

/**
 * SPI
 **/
void DEV_SPI_Init(void)
{
    // SPI Config
    // SPI1.setRX(LCD_MISO_PIN);
    SPI1.setCS(LCD_CS_PIN);
    SPI1.setSCK(LCD_CLK_PIN);
    SPI1.setTX(LCD_MOSI_PIN);
    SPI1.begin();
    SPI1.beginTransaction(SPISettings((SPI_BAUDRATE_MHZ * 1000U * 1000U), MSBFIRST, SPI_MODE0));
}

void DEV_SPI_deInit(void)
{
  SPI1.end();
}

void DEV_SPI_WriteByte(uint8_t Value)
{
    SPI1.transfer(Value);
}

void DEV_SPI_Write_nByte(uint8_t pData[], uint32_t Len)
{
    SPI1.transfer(pData, Len);
}

/**
 * I2C
 **/
void DEV_I2C_Init(void)
{
    // I2C Config
    Wire1.setSDA(DEV_SDA_PIN);
    Wire1.setSCL(DEV_SCL_PIN);
    Wire1.setClock((IIC_BAUDRATE_KHZ*1000U));
    Wire1.begin();
}

void DEV_I2C_deInit(void)
{
    Wire1.end();
}

void DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value)
{
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    Wire1.write(Value);
    Wire1.endTransmission();
}

void DEV_I2C_Write_nByte(uint8_t addr, uint8_t *pData, uint32_t Len)
{
    Wire1.beginTransmission(addr);
    Wire1.write(pData,Len);
    Wire1.endTransmission();
}

uint8_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg)
{
    uint8_t value;
  
    Wire1.beginTransmission(addr);
    Wire1.write((byte)reg);
    Wire1.endTransmission();
  
    Wire1.requestFrom(addr, (byte)1);
    value = Wire1.read();
  
    return value;
}
void DEV_I2C_Read_nByte(uint8_t addr,uint8_t reg, uint8_t *pData, uint32_t Len)
{
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    Wire1.endTransmission();
    
    Wire1.requestFrom(addr, Len);
  
    uint8_t i = 0;
    for(i = 0; i < Len; i++) {
      pData[i] =  Wire1.read();
    }
    Wire1.endTransmission();
}

/**
 * KEY
 **/
void DEV_KEY_Config(uint16_t Pin)
{
    pinMode(Pin,INPUT_PULLUP);
}

/**
 * IRQ
 **/
void DEV_IRQ_SET(uint gpio, uint32_t events, gpio_irq_callback_t callback)
{
    gpio_set_irq_enabled_with_callback(gpio,events,true,callback);
}

/**
 * PA Ctrl
 **/
void DEV_PA_Ctrl(void)
{
    gpio_init(PA_CTRL);
    gpio_set_dir(PA_CTRL, GPIO_OUT);
    gpio_put(PA_CTRL, 1);
} 

/**
 * lcd power pin
 **/
void DEV_LCD_Power_GPIO_Init(void)
{
    DEV_GPIO_Mode(LCD_PWR_PIN, 1);
    DEV_Digital_Write(LCD_PWR_PIN, 0);
}

void DEV_LCD_Power_Open(void)
{
    DEV_Digital_Write(LCD_PWR_PIN, 0);
}

void DEV_LCD_Power_Close(void)
{
    DEV_Digital_Write(LCD_PWR_PIN, 1);
}


/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
uint8_t DEV_Module_Init(void)
{
    // stdio Config
    DEV_Stdio_Init();

    // GPIO Config
    DEV_GPIO_Init();
    
    // ADC Config
    DEV_ADC_Init();

    // PWM Config
    // DEV_PWM_Init();

    // SPI Config
    DEV_SPI_Init();

    // I2C Config
    DEV_I2C_Init();

    printf("DEV_Module_Init OK \r\n");
    return 0;
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_Module_Exit(void)
{
    // DEV_PWM_deInit();
    DEV_SPI_deInit();
    DEV_I2C_deInit();
	DEV_GPIO_deInit();
    printf("DEV_Module_Exit OK \r\n");
}



