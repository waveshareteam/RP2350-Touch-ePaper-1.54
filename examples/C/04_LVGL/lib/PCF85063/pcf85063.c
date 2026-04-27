#include "pcf85063.h"
#include "DEV_Config.h"
#include <time.h>

static void pcf85063_reg_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    DEV_I2C_Read_nByte(SENSOR_I2C_PORT, PCF85063_DEVICE_ADDR, reg_addr, data, len);
}

static void pcf85063_reg_write_byte(uint8_t reg_addr, uint8_t *data, size_t len)
{
    uint8_t write_buffer[len + 1];
    memset(write_buffer, 0, sizeof(write_buffer));
    write_buffer[0] = reg_addr;
    memcpy(write_buffer + 1, data, len);
    DEV_I2C_Write_nByte(SENSOR_I2C_PORT, PCF85063_DEVICE_ADDR, write_buffer, (len+1));
}
 
void pcf85063_init(void)
{
    uint8_t seconds = 0;
    pcf85063_reg_read(PCF85063_SECONDS, (uint8_t *)&seconds, 1);
    if (seconds & 0x80)
        printf("oscillator_stop detected\n");
    else
        printf("RTC has beeing kept running!\n");
}

static uint8_t dec2bcd(uint8_t value)
{
    return ((value / 10) << 4) + (value % 10);
}

static uint8_t bcd2dec(uint8_t value)
{
    return (((value & 0xF0) >> 4) * 10) + (value & 0xF);
}

void pcf85063_get_time(struct tm *now_tm)
{
    uint8_t time_data[7] = {0};
    pcf85063_reg_read(PCF85063_SECONDS, time_data, 7);
    now_tm->tm_sec = bcd2dec(time_data[0] & 0x7F);
    now_tm->tm_min = bcd2dec(time_data[1] & 0x7F);
    now_tm->tm_hour = bcd2dec(time_data[2] & 0x3F);
    now_tm->tm_mday = bcd2dec(time_data[3] & 0x3F);
    now_tm->tm_wday = bcd2dec(time_data[4] & 0x7);
    now_tm->tm_mon = bcd2dec(time_data[5] & 0x1F) - 1;
    now_tm->tm_year = bcd2dec(time_data[6]) + 100;
}

void pcf85063_set_time(struct tm *now_tm)
{
    uint8_t time_data[7] = {0};
    mktime(now_tm);

    time_data[0] = dec2bcd(now_tm->tm_sec) & 0x7F;
    time_data[1] = dec2bcd(now_tm->tm_min) & 0x7F;
    time_data[2] = dec2bcd(now_tm->tm_hour) & 0x3F;
    time_data[3] = dec2bcd(now_tm->tm_mday) & 0x3F;
    time_data[4] = dec2bcd(now_tm->tm_wday) & 0x7;
    time_data[5] = dec2bcd(now_tm->tm_mon + 1) & 0x1F;
    time_data[6] = dec2bcd((now_tm->tm_year - 100) % 100);

    pcf85063_reg_write_byte(PCF85063_SECONDS, time_data, 7);
}

#if 0
static void pcf85063_task(void *arg)
{
    struct tm now_tm;
    // pcf85063_get_time(&now_tm);

    // if (now_tm.tm_year < 124 && now_tm.tm_year > 130 )
    {
        now_tm.tm_year = 2024 - 1900; // The year starts from 1900
        now_tm.tm_mon = 11 - 1;       // Months start from 0 (November = 10)
        now_tm.tm_mday = 22;          // Day of the month
        now_tm.tm_hour = 12;          // Hour
        now_tm.tm_min = 0;            // Minute
        now_tm.tm_sec = 0;            // Second
        now_tm.tm_isdst = -1;         // Automatically detect daylight saving time
        pcf85063_set_time(&now_tm);
    }

    while (1)
    {
        pcf85063_get_time(&now_tm);
        printf("time: %s\n", asctime(&now_tm));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void pcf85063_test(void)
{
    xTaskCreate(pcf85063_task, "pcf85063_task", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}
#endif

static bool pcf85063_check_alarm_param(const pcf85063_alarm_t *alarm)
{
    if (alarm == NULL) {
        printf("Alarm param check failed: NULL pointer\r\n");
        return false;
    }

    if (alarm->second.enable == ALARM_ENABLE) {
        if (alarm->second.value > 59) {
            printf("Invalid second value: %hhu (enable=Yes)\r\n", 
                   alarm->second.value);
            return false;
        }
    }

    if (alarm->minute.enable == ALARM_ENABLE) {
        if (alarm->minute.value > 59) {
            printf("Invalid minute value: %hhu (enable=Yes)\r\n", 
                   alarm->minute.value);
            return false;
        }
    }

    if (alarm->hour.enable == ALARM_ENABLE) {
        if (alarm->hour.value > 23) {
            printf("Invalid hour value: %hhu (enable=Yes)\r\n", 
                   alarm->hour.value);
            return false;
        }
    }

    if (alarm->day.enable == ALARM_ENABLE) {
        if (alarm->day.value < 1 || alarm->day.value > 31) {
            printf("Invalid day value: %hhu (enable=Yes)\r\n", 
                   alarm->day.value);
            return false;
        }
    }

    if (alarm->weekday.enable == ALARM_ENABLE) {
        if (alarm->weekday.value > 6) {
            printf("Invalid weekday value: %hhu (enable=Yes)\r\n", 
                   alarm->weekday.value);
            return false;
        }
    }

    return true;
}

void pcf85063_set_alarm(pcf85063_alarm_t *alarm)
{
    uint8_t alarm_data[5] = {0};

    if (!pcf85063_check_alarm_param(alarm)) {
        printf("Set alarm failed: invalid parameters\r\n");
        return;
    }
    
    alarm_data[0] = dec2bcd(alarm->second.value) & 0x7F;
    if (alarm->second.enable == ALARM_DISABLE) {
        alarm_data[0] |= 0x80;
    }
    
    alarm_data[1] = dec2bcd(alarm->minute.value) & 0x7F;
    if (alarm->minute.enable == ALARM_DISABLE) {
        alarm_data[1] |= 0x80;
    }
    
    alarm_data[2] = dec2bcd(alarm->hour.value) & 0x3F;
    if (alarm->hour.enable == ALARM_DISABLE) {
        alarm_data[2] |= 0x80;
    }
    
    alarm_data[3] = dec2bcd(alarm->day.value) & 0x3F;
    if (alarm->day.enable == ALARM_DISABLE) {
        alarm_data[3] |= 0x80;
    }
    
    alarm_data[4] = dec2bcd(alarm->weekday.value) & 0x7;
    if (alarm->weekday.enable == ALARM_DISABLE) {
        alarm_data[4] |= 0x80;
    }
    
    pcf85063_reg_write_byte(PCF85063_SECOND_ALARM, alarm_data, 5);
}

void pcf85063_get_alarm(pcf85063_alarm_t *alarm)
{
    uint8_t alarm_data[5] = {0};
    pcf85063_reg_read(PCF85063_SECOND_ALARM, alarm_data, 5);
    
    alarm->second.enable = (alarm_data[0] & 0x80) ? ALARM_DISABLE : ALARM_ENABLE;
    alarm->second.value = bcd2dec(alarm_data[0] & 0x7F);
    
    alarm->minute.enable = (alarm_data[1] & 0x80) ? ALARM_DISABLE : ALARM_ENABLE;
    alarm->minute.value = bcd2dec(alarm_data[1] & 0x7F);
    
    alarm->hour.enable = (alarm_data[2] & 0x80) ? ALARM_DISABLE : ALARM_ENABLE;
    alarm->hour.value = bcd2dec(alarm_data[2] & 0x3F);
    
    alarm->day.enable = (alarm_data[3] & 0x80) ? ALARM_DISABLE : ALARM_ENABLE;
    alarm->day.value = bcd2dec(alarm_data[3] & 0x3F);
    
    alarm->weekday.enable = (alarm_data[4] & 0x80) ? ALARM_DISABLE : ALARM_ENABLE;
    alarm->weekday.value = bcd2dec(alarm_data[4] & 0x7);
}

void pcf85063_print_alarm(const pcf85063_alarm_t *alarm)
{
    if (alarm == NULL) {
        printf("Alarm config is NULL\r\n");
        return;
    }

    printf("Alarm Configuration:\r\n");
    printf("  Second:    Value=%02d, Enable=%s\r\n",
           alarm->second.value,
           alarm->second.enable == ALARM_ENABLE ? "Yes" : "No");
    printf("  Minute:    Value=%02d, Enable=%s\r\n",
           alarm->minute.value,
           alarm->minute.enable == ALARM_ENABLE ? "Yes" : "No");
    printf("  Hour:      Value=%02d, Enable=%s\r\n",
           alarm->hour.value,
           alarm->hour.enable == ALARM_ENABLE ? "Yes" : "No");
    printf("  Day:       Value=%02d, Enable=%s\r\n",
           alarm->day.value,
           alarm->day.enable == ALARM_ENABLE ? "Yes" : "No");
    printf("  Weekday:   Value=%d (%s), Enable=%s\r\n",
           alarm->weekday.value,
           (alarm->weekday.value == 0) ? "Sun" :
           (alarm->weekday.value == 1) ? "Mon" :
           (alarm->weekday.value == 2) ? "Tue" :
           (alarm->weekday.value == 3) ? "Wed" :
           (alarm->weekday.value == 4) ? "Thu" :
           (alarm->weekday.value == 5) ? "Fri" :
           (alarm->weekday.value == 6) ? "Sat" : "Invalid",
           alarm->weekday.enable == ALARM_ENABLE ? "Yes" : "No");
}

bool pcf85063_check_alarm_flag(void)
{
    uint8_t ctrl2 = 0;
    pcf85063_reg_read(PCF85063_CONTROL_2, &ctrl2, 1);
    return (ctrl2 & 0x40) ? true : false;
}

void pcf85063_clear_alarm_flag(void)
{
    uint8_t ctrl2 = 0;
    pcf85063_reg_read(PCF85063_CONTROL_2, &ctrl2, 1);
    ctrl2 &= ~0x40;
    pcf85063_reg_write_byte(PCF85063_CONTROL_2, &ctrl2, 1);
}

void pcf85063_set_alarm_interrupt(bool enable)
{
    uint8_t ctrl2 = 0;
    
    pcf85063_reg_read(PCF85063_CONTROL_2, &ctrl2, 1);
    
    if (enable) {
        ctrl2 |= 0x80;
    } else {
        ctrl2 &= ~0x80;
    }

    pcf85063_reg_write_byte(PCF85063_CONTROL_2, &ctrl2, 1);
}

bool pcf85063_is_alarm_interrupt_enabled(void)
{
    uint8_t ctrl2 = 0;
    pcf85063_reg_read(PCF85063_CONTROL_2, &ctrl2, 1);
    return (ctrl2 & 0x80) ? true : false;
}

