#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"

#include "DEV_Config.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "FT6336U.h"

/*********************
 *      DEFINES
 *********************/


#define KEY_DEBOUNCE_CNT      1//2  
#define KEY_LONG_PRESS_CNT    20 
#define KEY_REPEAT_CNT        10 

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    KEY_NONE = 0, 
    KEY_LEFT,     
    KEY_POWER     
} key_type_t;

typedef enum {
    KEY_EVENT_NONE,       
    KEY_EVENT_SHORT_PRESS,
    KEY_EVENT_LONG_PRESS, 
    KEY_EVENT_LONG_REPEAT 
} key_event_type_t;

typedef struct {
    uint8_t press_cnt;     
    uint8_t release_cnt;   
    key_type_t key;        
    key_event_type_t event;
    uint8_t long_repeat_cnt;
} key_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static key_type_t key_scan_hw(void);
static void key_event_process(key_state_t *key_state, key_type_t curr_key);

static void ts_read_cb(lv_indev_drv_t * drv, lv_indev_data_t*data);
static void touch_callback(uint gpio, uint32_t events);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_keypad;
static key_state_t g_key_state = {0};
static key_type_t g_last_key = KEY_NONE;

static uint16_t g_last_reported_key = 0;


#if INPUTDEV_TS
static lv_indev_drv_t indev_ts = {0};
static volatile uint16_t ts_x = 0;
static volatile uint16_t ts_y = 0;
static lv_indev_state_t ts_act = LV_INDEV_STATE_RELEASED;
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;


#if INPUTDEV_TS
    FT6336U_Init(FT6336U_Point_Mode);
    // /*4.Init touch screen as input device*/ 
    lv_indev_drv_init(&indev_ts); 
    indev_ts.type = LV_INDEV_TYPE_POINTER;    
    indev_ts.read_cb = ts_read_cb;            
    lv_indev_t * ts_indev = lv_indev_drv_register(&indev_ts);
    DEV_KEY_Config(Touch_INT_PIN);
    //Enable touch IRQ
    DEV_IRQ_SET(Touch_INT_PIN, GPIO_IRQ_EDGE_RISE, &touch_callback);
#endif

    keypad_init();

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if INPUTDEV_TS
static void touch_callback(uint gpio, uint32_t events)
{
    if (gpio == Touch_INT_PIN)
    {
        FT6336U_Get_Point();
        ts_x = FT6336U.touch1_x;
        ts_y = FT6336U.touch1_y;
        ts_act = LV_INDEV_STATE_PRESSED;
    }
}

static void ts_read_cb(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
    data->point.x = ts_x;
    data->point.y = ts_y; 
    data->state = ts_act;

    if (ts_act == LV_INDEV_STATE_PRESSED) {
        printf("touch point: %d, %d\r\n", data->point.x, data->point.y);
    }

    ts_act = LV_INDEV_STATE_RELEASED;
}
#endif

bool __no_inline_not_in_flash_func(get_bootsel_button)() {
    const uint CS_PIN_INDEX = 1;

    // Must disable interrupts, as interrupt handlers may be in flash, and we
    // are about to temporarily disable flash access!
    uint32_t flags = save_and_disable_interrupts();

    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Note we can't call into any sleep functions in flash right now
    for (volatile int i = 0; i < 1000; ++i);

    // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
    // Note the button pulls the pin *low* when pressed.
    #define CS_BIT SIO_GPIO_HI_IN_QSPI_CSN_BITS

    bool button_state = !(sio_hw->gpio_hi_in & CS_BIT);

    // Need to restore the state of chip select, else we are going to have a
    // bad time when we return to code in flash!
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_state;
}

static void keypad_init(void)
{
    // printf(" [%s:%d] \n",__func__, __LINE__);
    DEV_KEY_Config(KEY_PWR_PIN);
}


#if 0

#define KEY_SCAN_PERIOD_MS      10 
#define KEY_DEBOUNCE_MS         30 
#define KEY_REPEAT_INTERVAL_MS  100 

#define DEBOUNCE_CNT            (KEY_DEBOUNCE_MS / KEY_SCAN_PERIOD_MS)
#define REPEAT_CNT              (KEY_REPEAT_INTERVAL_MS / KEY_SCAN_PERIOD_MS)

static key_type_t g_current_key = KEY_NONE;
static uint16_t g_press_duration = 0;      

static key_type_t key_scan_hw(void)
{
    if(get_bootsel_button()) {
        return KEY_LEFT;
    } else if(DEV_Digital_Read(KEY_PWR_PIN) == 0) {
        return KEY_POWER;
    }
    return KEY_NONE;
}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    key_type_t raw_key = key_scan_hw();
    data->state = LV_INDEV_STATE_REL;
    data->key = 0;

    if(raw_key != KEY_NONE) {
        if(g_current_key != raw_key) {
            g_current_key = raw_key;
            g_press_duration = 0;
        }

        g_press_duration++;

        if(g_press_duration < DEBOUNCE_CNT) {
            return;
        }

        if((g_press_duration == DEBOUNCE_CNT) ||
           ((g_press_duration - DEBOUNCE_CNT) % REPEAT_CNT == 0)) {

            // 映射到 LVGL 键值
            switch(g_current_key) {
                case KEY_LEFT:   data->key = LV_KEY_LEFT; break;
                case KEY_POWER:  data->key = LV_KEY_ENTER; break;
                default: return;
            }
            data->state = LV_INDEV_STATE_PR;
        }
    } else {
        if(g_current_key != KEY_NONE) {
            g_current_key = KEY_NONE;
            g_press_duration = 0;
        }
    }
}

#else

static key_type_t key_scan_hw(void)
{
    // printf(" [%s:%d] \n",__func__, __LINE__);

    if(get_bootsel_button()) {
        // printf(" [%s:%d]_LEFT \n",__func__, __LINE__);
        return KEY_LEFT;
    }else if(DEV_Digital_Read(KEY_PWR_PIN) == 0) {
        // printf(" [%s:%d]_POWER \n",__func__, __LINE__);
        return KEY_POWER;
    }
    else {
        return KEY_NONE;
    }
}

static void key_event_process(key_state_t *key_state, key_type_t curr_key)
{
    // printf(" [%s:%d] \n",__func__, __LINE__);
    if(curr_key == KEY_NONE) {
        if(key_state->press_cnt > 0) {
            key_state->release_cnt++;
    printf(" [%s:%d] \n",__func__, __LINE__);
            if(key_state->release_cnt >= KEY_DEBOUNCE_CNT) {
                if(key_state->press_cnt < KEY_LONG_PRESS_CNT) {
                    key_state->event = KEY_EVENT_SHORT_PRESS;
                }
                key_state->press_cnt = 0;
                key_state->release_cnt = 0;
                key_state->long_repeat_cnt = 0;
            }
        }
        else {
            key_state->event = KEY_EVENT_NONE;
        }
        return;
    }

    key_state->release_cnt = 0;
    key_state->press_cnt++;

    if(key_state->press_cnt == KEY_DEBOUNCE_CNT) {
        g_last_key = curr_key;
        key_state->event = KEY_EVENT_NONE;
    }
    else if(key_state->press_cnt == KEY_LONG_PRESS_CNT) {
        key_state->event = KEY_EVENT_LONG_PRESS;
        key_state->long_repeat_cnt = 0;
    }
    else if(key_state->press_cnt > KEY_LONG_PRESS_CNT) {
        key_state->long_repeat_cnt++;
        if(key_state->long_repeat_cnt >= KEY_REPEAT_CNT) {
            key_state->event = KEY_EVENT_LONG_REPEAT;
            key_state->long_repeat_cnt = 0;
        }
        else {
            key_state->event = KEY_EVENT_NONE;
        }
    }
    else {
        key_state->event = KEY_EVENT_NONE;
    }

}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    key_type_t curr_key = key_scan_hw();
    key_event_process(&g_key_state, curr_key);

    data->state = LV_INDEV_STATE_REL;
    data->key = 0;

    if(g_key_state.event == KEY_EVENT_SHORT_PRESS) {
        switch(g_last_key) {
            case KEY_LEFT:      data->key = LV_KEY_LEFT; break;
            case KEY_POWER:     data->key = LV_KEY_ENTER; break;
            default: return;
        }
        data->state = LV_INDEV_STATE_PR;
        g_last_reported_key = data->key;
    }
    else if(g_key_state.event == KEY_EVENT_LONG_PRESS && g_last_key == KEY_POWER) {
        data->key = LV_KEY_POWER_LONG;
        data->state = LV_INDEV_STATE_PR;
        g_last_reported_key = data->key;
    }
    else if(g_key_state.event == KEY_EVENT_LONG_REPEAT && g_last_key == KEY_POWER) {
        data->key = LV_KEY_POWER_REPEAT;
        data->state = LV_INDEV_STATE_PR;
        g_last_reported_key = data->key;
    }
    else if(curr_key == KEY_NONE && g_key_state.press_cnt == 0 && g_last_reported_key != 0) {
        data->key = g_last_reported_key;
        data->state = LV_INDEV_STATE_REL;
        g_last_reported_key = 0;
    }
}
#endif

#endif

