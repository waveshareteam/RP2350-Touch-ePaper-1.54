/*****************************************************************************
* | File      	:   FT6336U.h
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
#ifndef _FT6336U_H_
#define _FT6336U_H_

#include <stdint.h>
#include "hardware/i2c.h"
#include "pico/time.h"

#define FT6336U_I2C_ADDR  0x38

// Touch Parameter
#define FT6336U_PRES_DOWN 0x2
#define FT6336U_COORD_UD  0x1

// Registers
#define FT6336U_ADDR_DEVICE_MODE 	0x00
typedef enum {
    working_mode = 0b000,
    factory_mode = 0b100,
} DEVICE_MODE_Enum;
#define FT6336U_ADDR_TD_STATUS 		0x02

#define FT6336U_ADDR_TOUCH1_EVENT 	0x03
#define FT6336U_ADDR_TOUCH1_ID 		0x05
#define FT6336U_ADDR_TOUCH1_X 		0x03
#define FT6336U_ADDR_TOUCH1_Y 		0x05
#define FT6336U_ADDR_TOUCH1_WEIGHT  0x07
#define FT6336U_ADDR_TOUCH1_MISC    0x08

#define FT6336U_ADDR_TOUCH2_EVENT 	0x09
#define FT6336U_ADDR_TOUCH2_ID 		0x0B
#define FT6336U_ADDR_TOUCH2_X 		0x09
#define FT6336U_ADDR_TOUCH2_Y 		0x0B
#define FT6336U_ADDR_TOUCH2_WEIGHT  0x0D
#define FT6336U_ADDR_TOUCH2_MISC    0x0E

#define FT6336U_ADDR_THRESHOLD          0x80
#define FT6336U_ADDR_FILTER_COE         0x85
#define FT6336U_ADDR_CTRL               0x86
typedef enum {
    keep_active_mode = 0,
    switch_to_monitor_mode = 1,
} CTRL_MODE_Enum;
#define FT6336U_ADDR_TIME_ENTER_MONITOR 0x87
#define FT6336U_ADDR_ACTIVE_MODE_RATE   0x88
#define FT6336U_ADDR_MONITOR_MODE_RATE  0x89

#define FT6336U_ADDR_RADIAN_VALUE           0x91
#define FT6336U_ADDR_OFFSET_LEFT_RIGHT      0x92
#define FT6336U_ADDR_OFFSET_UP_DOWN         0x93
#define FT6336U_ADDR_DISTANCE_LEFT_RIGHT    0x94
#define FT6336U_ADDR_DISTANCE_UP_DOWN       0x95
#define FT6336U_ADDR_DISTANCE_ZOOM          0x96

#define FT6336U_ADDR_LIBRARY_VERSION_H  0xA1
#define FT6336U_ADDR_LIBRARY_VERSION_L  0xA2
#define FT6336U_ADDR_CHIP_ID            0xA3
#define FT6336U_ADDR_G_MODE             0xA4
typedef enum {
    pollingMode = 0,
    triggerMode = 1,
} G_MODE_Enum;
#define FT6336U_ADDR_POWER_MODE         0xA5
#define FT6336U_ADDR_FIRMARE_ID         0xA6
#define FT6336U_ADDR_FOCALTECH_ID       0xA8
#define FT6336U_ADDR_RELEASE_CODE_ID    0xAF
#define FT6336U_ADDR_STATE              0xBC

#define FT6336U_ADDR_GESTURE_EN		    0xD0
#define FT6336U_ADDR_GESTURE_ENABLE     0x01
#define FT6336U_ADDR_GESTURE_OUTPUT     0xD3
typedef enum {
    FT6336U_DEVICE_ON,
    FT6336U_DEVICE_OFF
} Device_State;

typedef enum
{
	FT6336U_Point_Mode = 1,
	FT6336U_Gesture_Mode,
} FT6336U_Mode;

typedef enum {
	FT6336U_GESTURE_ID,
    FT6336U_FINGER_NUMBER,
    FT6336U_TOUCH1_X ,
    FT6336U_TOUCH1_Y,
	FT6336U_TOUCH2_X,
    FT6336U_TOUCH2_Y,
} Value_Information;

typedef enum
{
	FT6336U_Gesture_None  = 0,
	FT6336U_Gesture_Left  = 0x20,
	FT6336U_Gesture_Right = 0x21,
	FT6336U_Gesture_Up    = 0x22,
	FT6336U_Gesture_Down  = 0x23,
	FT6336U_Gesture_Double_Click = 0x24,
} FT6336U_Gesture;

typedef struct
{
	uint8_t mode;
	uint16_t touch_num;
	uint16_t touch1_x;
	uint16_t touch1_y;
	uint16_t touch2_x;
	uint16_t touch2_y;
} FT6336U_Struct;

extern FT6336U_Struct FT6336U;

void FT6336U_Init(uint8_t mode);
void FT6336U_Reset();
uint16_t FT6336U_ReadID();
uint16_t FT6336U_ReadState(Value_Information info);
void FT6336U_Get_Point();
uint8_t FT6336U_Get_Gesture();

#endif // !_FT6336U_H_
