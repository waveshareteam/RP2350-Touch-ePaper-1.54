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
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

uint slice_num;

/**
 * System Clock
 **/
bool DEV_Sys_Clock_Init(void)
{
    if (!set_sys_clock_khz((SYS_CLOCK_MHZ*1000U),true)) {
        printf("set cpu clock falied!\n");
        return false;
    }

    return true;
}

/**
 * stdio
 **/
void DEV_Stdio_Init(void)
{
    stdio_init_all();
    // DEV_Delay_ms(2000);
}

void DEV_Stdio_deInit(void)
{
    stdio_deinit_all();
}

/**
 * delay
 **/
void DEV_Delay_ms(uint32_t xms)
{
    sleep_ms(xms);
}

void DEV_Delay_us(uint32_t xus)
{
    sleep_us(xus);
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
    gpio_deinit(LCD_RST_PIN);
    gpio_deinit(LCD_DC_PIN);
    gpio_deinit(LCD_CS_PIN);
	gpio_deinit(LCD_BUSY_PIN);
    printf("gpio_deinit OK \r\n");
}

void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode)
{
    gpio_init(Pin);
    if (Mode == 0 || Mode == GPIO_IN)
    {
        gpio_set_dir(Pin, GPIO_IN);
    }
    else
    {
        gpio_set_dir(Pin, GPIO_OUT);
    }
}

void DEV_Digital_Write(uint16_t Pin, uint8_t Value)
{
    gpio_put(Pin, Value);
}

uint8_t DEV_Digital_Read(uint16_t Pin)
{
    return gpio_get(Pin);
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
    adc_init(); 
    adc_gpio_init(BAT_ADC_PIN);
    adc_select_input(BAT_CHANNEL);
}

void DEV_ADC_deInit(void)
{

}

uint16_t DEV_ADC_Read(void)
{
    return adc_read();
}

/**
 * PWM
 **/
void DEV_PWM_Init(void)
{
    gpio_set_function(LCD_BL_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(LCD_BL_PIN);
    // pwm_set_output_polarity(slice_num,false,true);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 0);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_enabled(slice_num, true);
}

void DEV_PWM_deInit(void)
{
    pwm_set_enabled(slice_num, false);
    gpio_deinit(LCD_BL_PIN);
    printf("pwm_deinit OK \r\n");
}

void DEV_SET_PWM(uint8_t Value)
{
    if (Value < 0 || Value > 100)
    {
        printf("DEV_SET_PWM Error %d\r\n",Value);
    }
    else
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, Value);
    }
}

/**
 * SPI
 **/
void DEV_SPI_Init(void)
{
    spi_init(LCD_SPI_PORT, (SPI_BAUDRATE_MHZ * 1000U * 1000U));
    gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
}

void DEV_SPI_deInit(void)
{
    spi_deinit(LCD_SPI_PORT);
    gpio_deinit(LCD_CLK_PIN);
    gpio_deinit(LCD_MOSI_PIN);
    printf("spi_deinit OK \r\n");
}

void DEV_SPI_WriteByte(spi_inst_t *SPI_PORT,uint8_t Value)
{
    spi_write_blocking(SPI_PORT, &Value, 1);
}

void DEV_SPI_Write_nByte(spi_inst_t *SPI_PORT,uint8_t pData[], uint32_t Len)
{
    spi_write_blocking(SPI_PORT, pData, Len);
}

/**
 * I2C
 **/
void DEV_I2C_Init(void)
{
    i2c_init(SENSOR_I2C_PORT, (IIC_BAUDRATE_KHZ*1000U));
    gpio_set_function(DEV_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DEV_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DEV_SDA_PIN);
    gpio_pull_up(DEV_SCL_PIN);
}

void DEV_I2C_deInit(void)
{
    i2c_deinit(SENSOR_I2C_PORT);
    gpio_deinit(DEV_SDA_PIN);
    gpio_deinit(DEV_SCL_PIN);
    printf("iic_deinit OK \r\n");
}

void DEV_I2C_Write_Byte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t reg, uint8_t Value)
{
    uint8_t data[2] = {reg, Value};
    i2c_write_blocking(I2C_PORT, addr, data, 2, false);
}

void DEV_I2C_Write_nByte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t *pData, uint32_t Len)
{
    i2c_write_blocking(I2C_PORT, addr, pData, Len, false);
}

uint8_t DEV_I2C_Read_Byte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t reg)
{
    uint8_t buf = 0;
    i2c_write_blocking(I2C_PORT,addr,&reg,1,true);
    i2c_read_blocking(I2C_PORT,addr,&buf,1,false);
    return buf;
}
void DEV_I2C_Read_nByte(i2c_inst_t *I2C_PORT,uint8_t addr,uint8_t reg, uint8_t *pData, uint32_t Len)
{
    i2c_write_blocking(I2C_PORT,addr,&reg,1,true);
    i2c_read_blocking(I2C_PORT,addr,pData,Len,false);
}

/**
 * KEY
 **/
void DEV_KEY_Config(uint16_t Pin)
{
    gpio_init(Pin);
    gpio_pull_up(Pin);
    gpio_set_dir(Pin, GPIO_IN);
}

/**
 * WAKE_GPIO
 **/
void DEV_WAKE_GPIO_Config(void)
{
    gpio_init(WAKE_GPIO);
    gpio_set_dir(WAKE_GPIO, GPIO_IN);
    gpio_pull_up(WAKE_GPIO);
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



