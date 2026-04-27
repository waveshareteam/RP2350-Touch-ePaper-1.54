#ifndef __ADC__ABTTERY__
#define __ADC__ABTTERY__

#include "stdio.h"
#include "pico/stdlib.h"
#include <stdlib.h> //itoa()

#define BAT_R_UPPER (200000.0f)
#define BAT_R_LOWER (200000.0f)
#define ADC_REF_VOLTAGE (3.390f)
#define ADC_RESOLUTION (1 << 12)

typedef enum {
    CHARGE_STATE_CHARGING,  // Device is charging
    CHARGE_STATE_FULL       // Device is fully charged
} ChargeState;

typedef void (*ChargeFullCallback)(ChargeState state);

typedef struct {
    uint gpio_pin;               // GPIO pin for charge status
    bool full_level;             // Level indicating full charge
    ChargeState current_state;   // Current charging state
    ChargeFullCallback callback; // Callback for full charge event
    bool is_initialized;         // Initialization flag
} ChargeMonitor;

void battery_init(void);
void battery_read(float *voltage, uint16_t *adc_raw);

void battery_enable_pin_config(void);
void power_on_lock_battery_output(void);
void power_off_unlock_battery_output(void);

/**
 * Initialize the charge monitor
 * @param monitor Pointer to external ChargeMonitor object
 * @param gpio_pin GPIO pin number connected to charge IC status pin
 * @param full_level Expected level when fully charged (0 = low, 1 = high)
 * @return true if initialization succeeds, false otherwise
 */
bool charge_monitor_init(ChargeMonitor* monitor, uint gpio_pin, bool full_level);

/**
 * Set callback function for full charge event
 * @param monitor Pointer to ChargeMonitor object
 * @param callback Function to be called when full charge is detected
 */
void charge_monitor_set_callback(ChargeMonitor* monitor, ChargeFullCallback callback);

/**
 * Get current charging state
 * @param monitor Pointer to ChargeMonitor object
 * @return Current state (CHARGE_STATE_CHARGING or CHARGE_STATE_FULL)
 */
ChargeState charge_monitor_get_state(ChargeMonitor* monitor);

/**
 * Main monitoring loop (call in main loop or thread)
 * @param monitor Pointer to ChargeMonitor object
 * @param check_interval_ms Interval between status checks (in milliseconds)
 */
void charge_monitor_loop(ChargeMonitor* monitor, uint32_t check_interval_ms);

/**
 * Deinitialize the charge monitor
 * @param monitor Pointer to ChargeMonitor object
 */
void charge_monitor_deinit(ChargeMonitor* monitor);


#endif


