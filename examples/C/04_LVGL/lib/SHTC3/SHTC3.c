#include "SHTC3.h"

bool SHTC3_Write_Word(uint16_t Data)
{
    uint8_t buffer[2];
    buffer[0] = Data >> 8;
    buffer[1] = Data & 0xff;
    DEV_I2C_Write_nByte(SENSOR_I2C_PORT, SHTC3_I2C_ADDR, buffer, 2);
    return true;
}
bool SHTC3_Read(uint8_t *pData, uint8_t Len)
{
    i2c_read_blocking(SENSOR_I2C_PORT, SHTC3_I2C_ADDR, pData, Len, false);
    return true;
}

uint8_t SHTC3_crc8(uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xff;
    uint8_t poly = 0x31;
    while (len--)
    {
        crc ^= *data;
        
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ poly;
            }
            else
            {
                crc = crc << 1;
            }

        }
        data++;
    }
    return crc;
}

bool SHTC3_Sleep(void)
{
    SHTC3_Write_Word(SHTC3_REG_SLEEP);
    DEV_Delay_us(300);
}
bool SHTC3_Wake_Up(void)
{
    SHTC3_Write_Word(SHTC3_REG_WAKEUP);
    DEV_Delay_us(300);
}
bool SHTC3_Reset(void)
{
    SHTC3_Write_Word(SHTC3_REG_SOFTRESET);
    DEV_Delay_us(300);
}
int16_t SHTC3_Read_Id(void)
{
    uint8_t buffer[3];
    int16_t ID;
    SHTC3_Write_Word(SHTC3_REG_READID);
    SHTC3_Read(buffer, 3);
    ID = (buffer[0] << 8) + buffer[1];
    if (SHTC3_crc8(buffer, 2) == buffer[2])
    {
        printf("ID CRC PASS\r\n");
        return ID;
    }
    return -1;
}
bool SHTC3_Init()
{
    uint8_t buffer[4] = {0, 0, 0, 0};
    DEV_I2C_Write_nByte(SENSOR_I2C_PORT,SHTC3_I2C_ADDR,buffer,3);
    SHTC3_Wake_Up();
    return true;
    // self.read_id();
}
bool SHTC3_Measurement(float *temp, float *hum)
{
    uint8_t buffer[10];
    uint8_t temp_data_crc, hum_data_crc;
    uint16_t temp_data, hum_data;
    // uint16_t command =SHTC3_MEAS_ALL[SHTC3_STRETCH_MEAS][SHTC3_LOWPOWER_MEAS][SHTC3_HUM_FRIST_MEAS];
    uint16_t command = SHTC3_REG_NORMAL_T_F;
    SHTC3_Write_Word(command);
    if (SHTC3_LOWPOWER_MEAS)
    {
        DEV_Delay_ms(2);
    }
    else
    {
        DEV_Delay_ms(14);
    }

    SHTC3_Read(buffer, 6);
    temp_data_crc = SHTC3_crc8((buffer + SHTC3_HUM_FRIST_MEAS * 3), 2);
    hum_data_crc = SHTC3_crc8((buffer + 3 - SHTC3_HUM_FRIST_MEAS * 3), 2);

    if (temp_data_crc == buffer[3 * (SHTC3_HUM_FRIST_MEAS + 1) - 1] && hum_data_crc == buffer[3 * (2 - SHTC3_HUM_FRIST_MEAS) - 1])
    {
        temp_data = (buffer[SHTC3_HUM_FRIST_MEAS * 3] << 8) + buffer[SHTC3_HUM_FRIST_MEAS * 3 + 1];
        hum_data = (buffer[3 - SHTC3_HUM_FRIST_MEAS * 3] << 8) + buffer[4 - SHTC3_HUM_FRIST_MEAS * 3];
        *temp = (temp_data * 175.0) / (1 << 16) - 45;
        *hum = (hum_data * 100.0) / (1 << 16);
        return true;
    }
    printf("temp_data_crc = %x ,get_crc = %x\r\n", temp_data_crc, buffer[3 * (SHTC3_HUM_FRIST_MEAS + 1) - 1]);
    temp_data = (buffer[0] << 8) + buffer[1];
    hum_data =( buffer[3] << 8 )+ buffer[4];
    printf("\r\n 123 temp = %x ,hum = %x\r\n\r\n", temp_data, hum_data);

    *temp = 1;
    *hum = 1;
    return false;
}