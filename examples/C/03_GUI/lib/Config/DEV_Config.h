/*****************************************************************************
* | File      	:   DEV_Config.h
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
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include "stdio.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * system clock
 **/
#define SYS_CLOCK_MHZ (24U)

/**
 * spi
 **/
#define LCD_SPI_PORT    (spi1)
#define SPI_BAUDRATE_MHZ (4U)

/**
 * i2c
 **/
#define SENSOR_I2C_PORT (i2c1)
#define IIC_BAUDRATE_KHZ (400U)
#define DEV_SDA_PIN     (6)
#define DEV_SCL_PIN     (7)

/**
 * lcd
 **/
#define LCD_DC_PIN      (12)    //EPD_DC_PIN
#define LCD_CS_PIN      (9)     //EPD_CS_PIN
#define LCD_CLK_PIN     (10)    //EPD_SCLK_PIN
#define LCD_MOSI_PIN    (11)    //EPD_MOSI_PIN
// #define LCD_MISO_PIN    ()    //
#define LCD_RST_PIN     (14)    //EPD_RST_PIN
#define LCD_BL_PIN      (13)    //unUSE
#define LCD_PWR_PIN      (13)    //EPD_PWR_PIN -- Please note that this pin does not support the PWM function
#define LCD_BUSY_PIN    (15)    //EPD_BUSY_PIN

/**
 * touch
 **/
#define Touch_INT_PIN   (8)
#define Touch_RST_PIN   (16)

/**
 * IMU
 **/
// #define DOF_INT1        (23)
// #define DOF_INT2        ()

/**
 * Battery
 **/
#define BAT_CHANNEL     (3)
#define BAT_ADC_PIN     (29)
#define BAT_EN_PIN      (28)

/**
 * GPIO KEY
 **/
#define KEY_PWR_PIN     (24)

/**
 * lowPower
 **/
#define WAKE_GPIO       (17)   //Low-power wake-up pin

/**
 * PA NS4150B
 **/
#define PA_CTRL         (0)

/**
 * LED
 **/
#define LED_GREEN_PIN  (25)

/*------------------------------------------------------------------------------------------------------*/

bool DEV_Sys_Clock_Init(void);

void DEV_Stdio_Init(void);
void DEV_Stdio_deInit(void);

void DEV_Delay_ms(uint32_t xms);
void DEV_Delay_us(uint32_t xus);

void DEV_GPIO_Init(void);
void DEV_GPIO_deInit(void);
void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode);
void DEV_Digital_Write(uint16_t Pin, uint8_t Value);
uint8_t DEV_Digital_Read(uint16_t Pin);
void DEV_KEY_Config(uint16_t Pin);
void DEV_WAKE_GPIO_Config(void);

void DEV_LED_Init(void);
void DEV_LED_On(void);
void DEV_LED_Off(void);

void DEV_PA_Ctrl(void);

void DEV_LCD_Power_GPIO_Init(void);
void DEV_LCD_Power_Open(void);
void DEV_LCD_Power_Close(void);

void DEV_ADC_Init(void);
void DEV_ADC_deInit(void);
uint16_t DEV_ADC_Read(void);

void DEV_PWM_Init(void);
void DEV_PWM_deInit(void);
void DEV_SET_PWM(uint8_t Value);

void DEV_SPI_Init(void);
void DEV_SPI_deInit(void);
void DEV_SPI_WriteByte(spi_inst_t *SPI_PORT,uint8_t Value);
void DEV_SPI_Write_nByte(spi_inst_t *SPI_PORT,uint8_t *pData, uint32_t Len);

void DEV_I2C_Init(void);
void DEV_I2C_deInit(void);
void DEV_I2C_Write_Byte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t reg, uint8_t Value);
uint8_t DEV_I2C_Read_Byte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t reg);
void DEV_I2C_Write_nByte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t *pData, uint32_t Len);
void DEV_I2C_Read_nByte(i2c_inst_t *I2C_PORT,uint8_t addr,uint8_t reg, uint8_t *pData, uint32_t Len);

void DEV_IRQ_SET(uint gpio, uint32_t events, gpio_irq_callback_t callback);

uint8_t DEV_Module_Init(void);
void DEV_Module_Exit(void);

#endif



