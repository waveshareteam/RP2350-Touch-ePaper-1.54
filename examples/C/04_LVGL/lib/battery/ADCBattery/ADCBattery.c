#include "DEV_Config.h"
#include "ADCBattery.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#define BATTERY_ADC_SIZE 9

static void bubble_sort(uint16_t *data, uint16_t size)
{
    for (uint8_t i = 0; i < size - 1; i++)
    {
        for (uint8_t j = 0; j < size - i - 1; j++)
        {
            if (data[j] > data[j + 1])
            {
                uint16_t temp = data[j];
                data[j] = data[j + 1];
                data[j + 1] = temp;
            }
        }
    }
}

static uint16_t average_filter(uint16_t *samples)
{
    uint16_t out = 0;
    bubble_sort(samples, BATTERY_ADC_SIZE);
    for (int i = 1; i < BATTERY_ADC_SIZE - 1; i++)
    {
        out += samples[i] / (BATTERY_ADC_SIZE - 2);
    }
    return out;
}

static uint16_t battery_read_raw(void)
{
    uint16_t samples[BATTERY_ADC_SIZE];
    adc_select_input(BAT_CHANNEL);
    for (int i = 0; i < BATTERY_ADC_SIZE; i++)
    {
        samples[i] = DEV_ADC_Read();
    }
    return average_filter(samples);
}

void battery_read(float *voltage, uint16_t *adc_raw)
{
    static const uint16_t adc_resolution = ADC_RESOLUTION;

    static uint16_t result = 0;

    if (result != 0) {
        result = result * 0.7 + battery_read_raw() * 0.3;
    } else {
        result = battery_read_raw();
    }

    if (adc_raw)
    {
        *adc_raw = result;
    }
    if (voltage)
    {
        // *voltage = result * (3.394 / ((1 << 12)-1)) * 3.0;
        
        float v_adc = result * (ADC_REF_VOLTAGE / adc_resolution);
        *voltage = v_adc * (BAT_R_UPPER + BAT_R_LOWER) / BAT_R_LOWER;
    }
}

void battery_init(void)
{
    DEV_ADC_Init();
}

void battery_enable_pin_config(void)
{
    DEV_GPIO_Mode(BAT_EN_PIN, 1);
}

void power_on_lock_battery_output(void)
{
    DEV_Digital_Write(BAT_EN_PIN, 1);
}

void power_off_unlock_battery_output(void)
{
    DEV_Digital_Write(BAT_EN_PIN, 0);
}

bool charge_monitor_init(ChargeMonitor* monitor, uint gpio_pin, bool full_level) 
{
    // Check if monitor pointer is valid and not already initialized
    if (!monitor || monitor->is_initialized) {
        return false;
    }

    // Configure GPIO pin as input
    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_IN);
    // Uncomment below to enable pull-up/down if needed
    gpio_pull_up(gpio_pin);
    // gpio_pull_down(gpio_pin);

    // Initialize monitor parameters
    monitor->gpio_pin = gpio_pin;
    monitor->full_level = full_level;
    monitor->current_state = CHARGE_STATE_FULL;
    monitor->callback = NULL;
    monitor->is_initialized = true;

    return true;
}

void charge_monitor_set_callback(ChargeMonitor* monitor, ChargeFullCallback callback) 
{
    if (monitor && monitor->is_initialized) {
        monitor->callback = callback;
    }
}

ChargeState charge_monitor_get_state(ChargeMonitor* monitor) 
{
    if (!monitor || !monitor->is_initialized) {
        return CHARGE_STATE_CHARGING; // Default to charging if invalid
    }

    bool current_level = gpio_get(monitor->gpio_pin);
    return (current_level == monitor->full_level) ? CHARGE_STATE_FULL : CHARGE_STATE_CHARGING;
}

void charge_monitor_loop(ChargeMonitor* monitor, uint32_t check_interval_ms) 
{
    if (!monitor || !monitor->is_initialized) {
        return;
    }

    ChargeState new_state = charge_monitor_get_state(monitor);

    // Detect transition from charging to full
    if (new_state == CHARGE_STATE_FULL && monitor->current_state == CHARGE_STATE_CHARGING) {
        monitor->current_state = CHARGE_STATE_FULL;
        printf("Charging completed!\n");
        
        if (monitor->callback) {
            monitor->callback(monitor->current_state);
        }
    }
    // Detect transition from full to charging
    else if (new_state == CHARGE_STATE_CHARGING && monitor->current_state == CHARGE_STATE_FULL) {
        monitor->current_state = CHARGE_STATE_CHARGING;
        printf("Charging started...\n");
        
        if (monitor->callback) {
            monitor->callback(monitor->current_state);
        }
    }

    sleep_ms(check_interval_ms);
}

void charge_monitor_deinit(ChargeMonitor* monitor) 
{
    if (monitor) {
        monitor->is_initialized = false;
        monitor->callback = NULL;
        monitor->current_state = CHARGE_STATE_CHARGING;
        // GPIO is not deinitialized as it might be used elsewhere
    }
}

