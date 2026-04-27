#ifndef __BSP_PCF85063_H__
#define __BSP_PCF85063_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include <time.h>

#define PCF85063_DEVICE_ADDR 0x51


typedef enum
{
    PCF85063_CONTROL_1 = 0,
    PCF85063_CONTROL_2,
    PCF85063_OFFSET,
    PCF85063_RAM_BYTE,
    PCF85063_SECONDS,
    PCF85063_MINUTES,
    PCF85063_HOURS,
    PCF85063_DAYS,
    PCF85063_WEEKDAYS,
    PCF85063_MONTHS,
    PCF85063_YEARS,
    PCF85063_SECOND_ALARM,
    PCF85063_MINUTE_ALARM,
    PCF85063_HOUR_ALARM,
    PCF85063_DAY_ALARM,
    PCF85063_WEEKDAY_ALARM,
    PCF85063_TIMER_VALUE,
    PCF85063_TIMER_MODE,
}pcf85063_reg_t;

/**
 * 闹钟使能标志位定义
 */
typedef enum {
    ALARM_DISABLE = 0,
    ALARM_ENABLE  = 1
} pcf85063_alarm_en_t;

/**
 * 闹钟配置结构体
 * 每个字段包含值和使能标志
 * 当使能标志为ALARM_ENABLE时，对应字段参与闹钟匹配
 */
typedef struct {
    struct {
        uint8_t value;               // 秒值 (0-59)
        pcf85063_alarm_en_t enable;  // 使能标志
    } second;
    struct {
        uint8_t value;               // 分值 (0-59)
        pcf85063_alarm_en_t enable;
    } minute;
    struct {
        uint8_t value;               // 小时值 (0-23)
        pcf85063_alarm_en_t enable;
    } hour;
    struct {
        uint8_t value;               // 日期值 (1-31)
        pcf85063_alarm_en_t enable;
    } day;
    struct {
        uint8_t value;               // 星期值 (0-6)
        pcf85063_alarm_en_t enable;
    } weekday;
} pcf85063_alarm_t;

/**
 * 闹钟中断配置结构体
 */
typedef struct {
    bool alarm_irq_enable;          // 闹钟中断使能
    bool irq_polarity;              // 中断极性: false=低电平, true=高电平
} pcf85063_alarm_irq_t;

// 闹钟功能函数声明
void pcf85063_set_alarm(pcf85063_alarm_t *alarm);
void pcf85063_get_alarm(pcf85063_alarm_t *alarm);
void pcf85063_print_alarm(const pcf85063_alarm_t *alarm);

void pcf85063_set_alarm_interrupt(bool enable);
bool pcf85063_is_alarm_interrupt_enabled(void);

bool pcf85063_check_alarm_flag(void);
void pcf85063_clear_alarm_flag(void);

void pcf85063_init(void);
void pcf85063_get_time(struct tm *now_tm);
void pcf85063_set_time(struct tm *now_tm);
// void pcf85063_test(void);

#endif