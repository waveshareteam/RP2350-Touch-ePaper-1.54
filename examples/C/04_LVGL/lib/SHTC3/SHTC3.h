
#ifndef _SHTC3_H
#define _SHTC3_H

#include "DEV_Config.h"
#include <stdint.h>
#include <stdlib.h> //itoa()
#include <stdio.h>

#define SHTC3_I2C_ADDR (0X70)

#define SHTC3_REG_SLEEP                 (0xB098)    // Enter sleep mode
#define SHTC3_REG_WAKEUP                (0x3517)    // Wakeup mode
#define SHTC3_REG_SOFTRESET             (0x805D)    // Soft Reset
#define SHTC3_REG_READID                (0xEFC8)    // Read Out of ID Register

#define SHTC3_REG_NORMAL_T_F            (0x7866)    // Read T First And Clock Stretching Disabled In Normal Mode
#define SHTC3_REG_NORMAL_H_F            (0x58E0)    // Read H First And Clock Stretching Disabled In Normal Mode

#define SHTC3_REG_NORMAL_T_F_STRETCH    (0x7CA2)    // Read T First And Clock Stretching Enabled In Normal Mode
#define SHTC3_REG_NORMAL_H_F_STRETCH    (0x5C24)    // Read H First And Clock Stretching Enabled In Normal Mode

#define SHTC3_REG_LOWPOWER_T_F          (0x609C)    // Read T First And Clock Stretching Disabled In Lowpower Mode
#define SHTC3_REG_LOWPOWER_H_F          (0x401A)    // Read T First And Clock Stretching Disabled In Lowpower Mode

#define SHTC3_REG_LOWPOWER_T_F_STRETCH  (0x6458)    // Read T First And Clock Stretching Enabled In Lowpower Mode
#define SHTC3_REG_LOWPOWER_H_F_STRETCH  (0x44DE)    // Read T First And Clock Stretching Enabled In Lowpower Mode


// const uint16_t SHTC3_NORMAL_MEAS[2]                 = {SHTC3_REG_NORMAL_T_F,SHTC3_REG_NORMAL_H_F};
// const uint16_t SHTC3_NORMAL_MEAS_STRETCH[2]         = {SHTC3_REG_NORMAL_T_F_STRETCH,SHTC3_REG_NORMAL_H_F_STRETCH};
// const uint16_t SHTC3_LOWPOWER_MEAS[2]               = {SHTC3_REG_LOWPOWER_T_F,SHTC3_REG_LOWPOWER_H_F};
// const uint16_t SHTC3_LOWPOWER_MEAS_STRETCH[2]       = {SHTC3_REG_LOWPOWER_T_F_STRETCH,SHTC3_REG_LOWPOWER_H_F_STRETCH};

// const uint16_t *SHTC3_MEAS[2]                        = {SHTC3_NORMAL_MEAS,SHTC3_LOWPOWER_MEAS};
// const uint16_t *SHTC3_MEAS_STRETCH[2]                = {SHTC3_NORMAL_MEAS_STRETCH,SHTC3_LOWPOWER_MEAS_STRETCH};

// const uint16_t **SHTC3_MEAS_ALL[2]                    = {SHTC3_MEAS,SHTC3_MEAS_STRETCH};


#define SHTC3_HUM_FRIST_MEAS (0)
#define SHTC3_LOWPOWER_MEAS  (0)
#define SHTC3_STRETCH_MEAS   (0)


bool SHTC3_Init(void);
bool SHTC3_Sleep(void);
bool SHTC3_Wake_Up(void);
bool SHTC3_Reset(void);
int16_t SHTC3_Read_Id(void);
bool SHTC3_Measurement(float *temp,float *hum);
uint8_t SHTC3_crc8(uint8_t *data, uint16_t len);

#endif
