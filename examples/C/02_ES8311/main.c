/*****************************************************************************
* | File      	:   main.c
* | Author      :   Waveshare Team
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2026-04-01
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
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
#
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DEV_Config.h"
#include "audio_pio.h"
#include "es8311.h"

#include "hardware/watchdog.h"
#include "pico/multicore.h"  
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

uint8_t i2c_lock = 0;
#define I2C_LOCK() i2c_lock = 1
#define I2C_UNLOCK() i2c_lock = 0

// KEY state
typedef enum {
    KEY_STATE_NONE,
    KEY_STATE_WAIT_LONG_PRESS,
    KEY_STATE_LONG_PRESS,
} key_state_t;

key_state_t key_state = KEY_STATE_NONE;
struct repeating_timer long_press_timer;

/**
 * @brief Callback function for long key press detection
 * @param t Pointer to the repeating timer structure
 * @return bool True to keep the timer running, false to stop it
 */
static bool __no_inline_not_in_flash_func(repeating_long_press_cb)(struct repeating_timer *t)
{
    if(key_state == KEY_STATE_LONG_PRESS || key_state == KEY_STATE_WAIT_LONG_PRESS) // long press
    {
        if(key_state == KEY_STATE_WAIT_LONG_PRESS)
        {
            key_state = KEY_STATE_LONG_PRESS; 
            cancel_repeating_timer(&long_press_timer);
            add_repeating_timer_ms(200, repeating_long_press_cb, NULL, &long_press_timer);
        }

        pico_audio.volume--; // volume down
        if(pico_audio.volume < 0)
            pico_audio.volume = 0;
        
        I2C_LOCK();
        Es8311_Voice_Volume_Set(pico_audio.volume);
        I2C_UNLOCK();
        printf("Volume:%d\r\n",pico_audio.volume);
    }
    else
    {
        cancel_repeating_timer(&long_press_timer);
        key_state = KEY_STATE_NONE; 
    }
    return true;
}

/**
 * @brief GPIO interrupt callback for key press events
 * @param gpio GPIO pin number
 * @param events GPIO event flags indicating which events triggered the interrupt
 */
static void __no_inline_not_in_flash_func(key_callback)(uint gpio, uint32_t events)
{
    while(i2c_lock);
    if (gpio == KEY_PLUS)
    {
        if(events == GPIO_IRQ_EDGE_RISE) // key release
        {
            cancel_repeating_timer(&long_press_timer);

            if(key_state != KEY_STATE_LONG_PRESS) // short press
            {
                pico_audio.volume++; // volume up
                if(pico_audio.volume > 100)
                    pico_audio.volume = 100;

                Es8311_Voice_Volume_Set(pico_audio.volume);
                printf("Volume:%d\r\n",pico_audio.volume);
            }

            key_state = KEY_STATE_NONE; 
        }
        else // key press
        {
            if(key_state == KEY_STATE_NONE)
            {
                key_state = KEY_STATE_WAIT_LONG_PRESS; // wait long press
                add_repeating_timer_ms(300, repeating_long_press_cb, NULL, &long_press_timer);
            }
        }
    }
    else if (gpio == KEY_PWR)
    {
        watchdog_reboot(0,0,0);
    }
}

/**
 * @brief Main application entry point
 * @return int Application exit code, always returns 0
 */
int main() 
{  
    DEV_Module_Init();
    Es8311_Init(pico_audio);
    Es8311_Sample_Frequency_Config(pico_audio.mclk_freq, pico_audio.sample_freq);
    Es8311_Microphone_Config();
    Es8311_Voice_Volume_Set(pico_audio.volume);
    Es8311_Microphone_Gain_Set(pico_audio.mic_gain);

    uint16_t chip_id = Es8311_Read_Id();
    printf("Chip ID:0x%x\r\n", chip_id);

    DEV_SET_IRQ(KEY_PLUS, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, &key_callback);
    DEV_SET_IRQ(KEY_PWR, GPIO_IRQ_EDGE_FALL, &key_callback);

    // 1.Output 440HZ sine wave test
    // Sine_440hz_Out();
    
    // 2.Play Happy Birthday
    // Happy_Birthday_Out();

    // 3.Recording and playback loopback test
    Loopback_Test();

    //4.Play music
    // Music_Out();

    return 0;
}
