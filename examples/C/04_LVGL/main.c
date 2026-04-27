#include "DEV_Config.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

#include "lv_app_hwtest.h"

#include "ADCBattery.h"
#include "SDCard.h"

#include "audio_pio.h"
#include "es8311.h"

#include "pcf85063.h"
#include "SHTC3.h"

#include "hardware/watchdog.h"
#include "hardware/adc.h"



#define LVGL_TICK_PERIOD_MS 10

int32_t *mic_sample_buffers = NULL;
int mic_num_samples = 120000/2;

static bool repeating_lvgl_timer_cb(struct repeating_timer *t)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
    return true;
}

static void es8311_audio_init(void)
{
    DEV_PA_Ctrl();
    
    es8311_init(pico_audio);
    es8311_sample_frequency_config(pico_audio.mclk_freq, pico_audio.sample_freq);
    es8311_microphone_config();
    es8311_voice_volume_set(pico_audio.volume);
    es8311_microphone_gain_set(pico_audio.mic_gain);

    uint16_t chip_id = es8311_read_id();
    printf("Chip ID:0x%x\r\n", chip_id);

    mic_sample_buffers = malloc(mic_num_samples * sizeof(int32_t));
    if (mic_sample_buffers == NULL) {
        printf(" [%s:%d] buffers err!!!!!!! mic_num_samples:%d \n", __func__, __LINE__,mic_num_samples);
        while(1);
    }

    memset(mic_sample_buffers, 0, mic_num_samples * sizeof(int32_t)); 

    //MCLK
    mclk_pio_init();
    //READ
    din_pio_init();

    //WRITE
    dout_pio_init();
    pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, false);
}

static void pcf85063_set_time_and_alarm(void)
{
    struct tm now_tm = {0};
    pcf85063_init();
    pcf85063_get_time(&now_tm);
    printf("get start time: %s\n", asctime(&now_tm));

    now_tm.tm_year = 2026 - 1900; // The year starts from 1900
    now_tm.tm_mon = 1 - 1;       // Months start from 0 (November = 10)
    now_tm.tm_mday = 1;          // Day of the month
    now_tm.tm_hour = 12;          // Hour
    now_tm.tm_min = 0;            // Minute
    now_tm.tm_sec = 0;            // Second
    now_tm.tm_isdst = -1;         // Automatically detect daylight saving time
    pcf85063_set_time(&now_tm);

    if (pcf85063_check_alarm_flag()) {
        pcf85063_clear_alarm_flag();
        printf("clear alarm flag!\n");
    }

    pcf85063_alarm_t alarm = {
        .day     = {.value =  0,                  .enable = ALARM_DISABLE},
        .weekday = {.value =  0,                  .enable = ALARM_DISABLE},
        .hour    = {.value =  now_tm.tm_hour,     .enable = ALARM_ENABLE},
        .minute  = {.value =  now_tm.tm_min,      .enable = ALARM_ENABLE},
        .second  = {.value =  (now_tm.tm_sec+5), .enable = ALARM_ENABLE},
    };
    pcf85063_set_alarm(&alarm);
    printf("set alarm!\r\n");

    pcf85063_alarm_t get_alarm = {0};
    pcf85063_get_alarm(&get_alarm);
    pcf85063_print_alarm(&get_alarm);

    pcf85063_set_alarm_interrupt(true);

    if (pcf85063_is_alarm_interrupt_enabled()) {
        printf("palarm_interrupt_enabled succeed!\n");
    } else {
        printf("palarm_interrupt_enabled failed!\n");
    }
}

void power_pin_callback(uint gpio, uint32_t events) 
{
    DEV_Digital_Write(BAT_EN_PIN, 0);
}

int main(void)
{
    static struct repeating_timer lvgl_timer = {0};

    DEV_GPIO_Mode(BAT_EN_PIN, 1);
    DEV_Digital_Write(BAT_EN_PIN, 1);

    DEV_LED_Init();
    DEV_LED_On();
    DEV_WAKE_GPIO_Config();

    DEV_Module_Init();

    bool is_watchdog_reboot = watchdog_enable_caused_reboot();
    printf("Watchdog reboot: %s\r\n", is_watchdog_reboot ? "true" : "false");

    if (is_watchdog_reboot) {

        es8311_audio_init();
        DEV_Delay_ms(100);
        DEV_KEY_Config(KEY_PWR_PIN);
        DEV_IRQ_SET(KEY_PWR_PIN, GPIO_IRQ_EDGE_FALL, &power_pin_callback);

        while (1) {
            //READ
            pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din, true);
            for (int i = 0; i < mic_num_samples; i ++)
                mic_sample_buffers[i] = pio_sm_get_blocking(pico_audio.pio_1, pico_audio.sm_din);
            pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din, false);

            //WRITE
            pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, true);
            for(int i = 0; i < mic_num_samples; i++)
                pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, mic_sample_buffers[i]);
            pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, false);
        }
    }

    battery_init();
    adc_set_temp_sensor_enabled(true);

    SHTC3_Init();
    sd_card_init();
    pcf85063_set_time_and_alarm();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    add_repeating_timer_ms(LVGL_TICK_PERIOD_MS, repeating_lvgl_timer_cb, NULL, &lvgl_timer);

    lv_app_test_init();

    while (true) {
        lv_timer_handler();
        DEV_Delay_ms(LVGL_TICK_PERIOD_MS);
    }
    return 0;
}


