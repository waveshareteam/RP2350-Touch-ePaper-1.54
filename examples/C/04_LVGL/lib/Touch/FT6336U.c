/*****************************************************************************
* | File      	:   FT6336U.c
* | Author      :   Waveshare Team
* | Function    :   FT6336U Interface Functions
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-03-20
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
#include "FT6336U.h"
#include "DEV_Config.h"

FT6336U_Struct FT6336U;

/******************************************************************************
function :	Send one byte of data to the specified register of FT6336U
parameter:
******************************************************************************/
static void FT6336U_I2C_Write_Byte(uint8_t reg, uint8_t value) {
    DEV_I2C_Write_Byte(SENSOR_I2C_PORT, FT6336U_I2C_ADDR, reg, value);
}

/******************************************************************************
function :	Read one byte of data from the specified register of FT6336U
parameter:
******************************************************************************/
static uint8_t FT6336U_I2C_Read_Byte(uint8_t reg) {
    uint8_t value;
    value = DEV_I2C_Read_Byte(SENSOR_I2C_PORT, FT6336U_I2C_ADDR,reg);
    return value;
}

/******************************************************************************
function :	Read n byte of data from the specified register of FT6336U
parameter:
******************************************************************************/
static void FT6336U_I2C_Read_nByte(uint8_t reg, uint8_t *pData, uint32_t Len) {
    DEV_I2C_Read_nByte(SENSOR_I2C_PORT, FT6336U_I2C_ADDR,reg,pData,Len);
}

/******************************************************************************
function :	Reset the FT6336U
parameter:
******************************************************************************/
void FT6336U_Reset() {
    gpio_put(Touch_RST_PIN, 1);
    sleep_ms(10);
    gpio_put(Touch_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(Touch_RST_PIN, 1);
    sleep_ms(300);
}

/******************************************************************************
function :	Initialize the FT6336U
parameter:
        mode    ：  FT6336U_Point_Mode
                    FT6336U_Gesture_Mode
******************************************************************************/
void FT6336U_Init(uint8_t mode) {
    // FT6336U Reset
    FT6336U_Reset();

    FT6336U.mode = mode;
    if(mode == FT6336U_Gesture_Mode) {
        FT6336U_I2C_Write_Byte(FT6336U_ADDR_GESTURE_EN, FT6336U_ADDR_GESTURE_ENABLE);
    }
    else{
        FT6336U_I2C_Write_Byte(FT6336U_ADDR_GESTURE_EN, 0x00);
    }

    int32_t id = FT6336U_ReadID();
    printf("FT6336URegister_WhoAmI = %d\n", id);
    if(id != 0x64) { 
        printf("Invalid device ID: 0x%x\n", id);
        return;
    }
    printf("FT6336U initialized successfully\n");
}

/******************************************************************************
function :	Read the ID of FT6336U
parameter:
******************************************************************************/
uint16_t FT6336U_ReadID() {
    uint8_t id;
    id = FT6336U_I2C_Read_Byte(FT6336U_ADDR_CHIP_ID);
    return id;
}

/******************************************************************************
function :	Read the current status of FT6336U
parameter:
******************************************************************************/
uint16_t FT6336U_ReadState(Value_Information info) {
    uint8_t buf[2];
    
    switch(info) {
        case FT6336U_GESTURE_ID:
            buf[0] = FT6336U_I2C_Read_Byte(FT6336U_ADDR_GESTURE_OUTPUT);
            return buf[0];

        case FT6336U_FINGER_NUMBER:
            buf[0] = FT6336U_I2C_Read_Byte(FT6336U_ADDR_TD_STATUS);
            return buf[0];

        case FT6336U_TOUCH1_X:
            FT6336U_I2C_Read_nByte(FT6336U_ADDR_TOUCH1_X, buf, 2);
            return ((int16_t)(buf[0] & 0x0F) << 8) | buf[1];
            
        case FT6336U_TOUCH1_Y:
            FT6336U_I2C_Read_nByte(FT6336U_ADDR_TOUCH1_Y, buf, 2);
            return ((int16_t)(buf[0] & 0x0F) << 8) | buf[1];

        case FT6336U_TOUCH2_X:
            FT6336U_I2C_Read_nByte(FT6336U_ADDR_TOUCH2_X, buf, 2);
            return ((int16_t)(buf[0] & 0x0F) << 8) | buf[1];
            
        case FT6336U_TOUCH2_Y:
            FT6336U_I2C_Read_nByte(FT6336U_ADDR_TOUCH2_Y, buf, 2);
            return ((int16_t)(buf[0] & 0x0F) << 8) | buf[1];
    }
    return -1;
}

/******************************************************************************
function :	Get the coordinate value of FT6336U contact
parameter:
******************************************************************************/
void FT6336U_Get_Point() {
    uint8_t fingers = (uint8_t)FT6336U_ReadState(FT6336U_FINGER_NUMBER);
    if(fingers != 0) 
    {
        FT6336U.touch1_x = (int)FT6336U_ReadState(FT6336U_TOUCH1_X);
        FT6336U.touch1_y = (int)FT6336U_ReadState(FT6336U_TOUCH1_Y);
    }
}

/******************************************************************************
function :	Get the coordinate value of FT6336U contact
parameter:
******************************************************************************/
uint8_t FT6336U_Get_Gesture() {
    uint8_t gesture = FT6336U_ReadState(FT6336U_GESTURE_ID);
    return gesture;
}
