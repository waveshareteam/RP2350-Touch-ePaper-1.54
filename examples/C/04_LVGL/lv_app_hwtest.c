#include "lvgl.h"
#include "lv_port_indev.h"
#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "ADCBattery.h"
#include "SDCard.h"

#include "pcf85063.h"
#include "SHTC3.h"

#include "audio_pio.h"
#include "es8311.h"

#include "EPD_1in54_V2.h"

#include "lv_app_hwtest.h"


typedef enum {
    PAGE_SYS_STATUS = 1, //85063 battery humiture
    PAGE_AUDIO_TEST,
    PAGE_TOUCH_ENTRY, 
    PAGE_COLOR_TEST,
    PAGE_SD_CARD_TEST, 
    PAGE_MAX = PAGE_SD_CARD_TEST
} test_page_t;

static test_page_t current_page = PAGE_SYS_STATUS;
static lv_obj_t *pages[PAGE_MAX + 1] = {NULL};
static lv_obj_t *page_power_off = NULL;

static lv_obj_t *label_sys_info = NULL;
static lv_obj_t *label_battery_info = NULL;
static lv_obj_t *label_humiture_info = NULL;
static lv_obj_t *label_time_info = NULL;

static lv_obj_t *label_sd_info = NULL;
static lv_obj_t *color_title = NULL;
static lv_obj_t *label_color_name = NULL;
static lv_obj_t *label_audio_info = NULL;

static bool sd_card_mounted = true;
static uint32_t sd_card_capacity = 1024UL * 1024 * 1024; // 1GB
static bool sd_test_passed = false;
static uint8_t color_index = 0;

static lv_obj_t *label_touch_entry_info = NULL;
static void touch_entry_click_cb(lv_event_t *e);

static lv_timer_t *sys_update_timer = NULL;
static lv_timer_t *led_blink_timer = NULL;

extern lv_indev_t *indev_keypad;

 
static void create_touch_entry_page(void) 
{
    lv_obj_t *scr = lv_obj_create(NULL);
    pages[PAGE_TOUCH_ENTRY] = scr;
    
    label_touch_entry_info = lv_label_create(scr);
    lv_label_set_text(label_touch_entry_info, "Touch Screen Test\n\nTap screen to start");
    lv_obj_align(label_touch_entry_info, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_event_cb(scr, touch_entry_click_cb, LV_EVENT_CLICKED, NULL);
}

static void touch_entry_click_cb(lv_event_t *e) 
{
    printf("Start Touch Test\n");

    lv_obj_t *scr = lv_event_get_target(e);
    lv_obj_remove_event_cb(scr, touch_entry_click_cb);
    create_touch_test_ui();
}

static float read_chip_temp(void)
{
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = ADC_REF_VOLTAGE / (1 << 12);
    adc_select_input(4);
    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;
    return tempC;
}

static void read_battery_voltage(float *voltage, uint16_t *adc_raw) {
    battery_read(voltage, adc_raw);
    printf("voltage: %.2f V adc_raw: %d\n", *voltage, *adc_raw);
}

static uint32_t get_flash_size(void) {
    uint8_t txbuf[4] = {0x9F, 0, 0, 0};
    uint8_t rxbuf[4] = {0};
    uint32_t ints = save_and_disable_interrupts();
    flash_do_cmd(txbuf, rxbuf, 4);
    restore_interrupts(ints);
    return (1 << rxbuf[3]);
}

static void led_blink_timer_cb(lv_timer_t *timer)
{
    static bool led_state = false;
    led_state = !led_state;
    if (led_state) {
        DEV_LED_On();
    } else {
        DEV_LED_Off();
    }
}

static void sys_timer_cb(lv_timer_t *timer) {
    if (!label_sys_info) return;

    float chip_temp = read_chip_temp();
    uint32_t sys_clk = clock_get_hz(clk_sys) / 1000000; // MHz
    uint32_t flash_size = get_flash_size() / (1024 * 1024); // MB

    //system
    char sys_buf[200] = {0};
    snprintf(sys_buf, sizeof(sys_buf),
        "Chip: RP2350A\n"
        "CPU: %lu MHz\n"
        "Chip_Temp: %.1f °C\n"
        "RAM: 520 KB ; Flash: %lu MB",
        sys_clk, chip_temp, flash_size
    );
    lv_label_set_text(label_sys_info, sys_buf);

    //battery
    float vbat = 0.0;
    uint16_t vbat_adc_raw = 0;
    read_battery_voltage(&vbat, &vbat_adc_raw);

    char bat_buf[200] = {0};
    snprintf(bat_buf, sizeof(bat_buf),
        "Battery: %.2f V\n"
        "Battery_adc: %lu",
        vbat, vbat_adc_raw
    );
    lv_label_set_text(label_battery_info, bat_buf);

    //humiture
    float temp = 0.0; 
    float hum = 0.0;
    int16_t id = -1;
    bool crc_res = false;
    char hum_buf[200] = {0};

    crc_res = SHTC3_Measurement(&temp, &hum);  
    id = SHTC3_Read_Id();
    if (id > -1 && crc_res != false) {
        snprintf(hum_buf, sizeof(hum_buf),
            "sthc3_temp: %.2f °C\n"
            "sthc3_humidity: %.2f %%",
            temp, hum
        );
        lv_label_set_text(label_humiture_info, hum_buf);
    }

    //pcf85063
    struct tm cur_tm = {0};
    static bool is_alarm_ok = false;

    pcf85063_get_time(&cur_tm);
    printf("current time: %s\n", asctime(&cur_tm));

    if (pcf85063_check_alarm_flag() && 0 == DEV_Digital_Read(WAKE_GPIO)) {
        printf("Alarm triggered\n");
        is_alarm_ok = true;
        pcf85063_clear_alarm_flag();
    }

    lv_label_set_text_fmt( label_time_info, 
                          "data: %04d-%02d-%02d\ntime: %02d:%02d:%02d\npcf85063 test: %s",
                          (cur_tm.tm_year + 1900), 
                          (cur_tm.tm_mon + 1), 
                           cur_tm.tm_mday, 
                           cur_tm.tm_hour, 
                           cur_tm.tm_min, 
                           cur_tm.tm_sec, 
                           (is_alarm_ok ? "OK" : "ERROR")
                         );

    printf(" [%s:%d]\n%s\n", __func__, __LINE__, sys_buf);
    printf(" [%s:%d]\n%s\n", __func__, __LINE__, bat_buf);
    printf(" [%s:%d]\n%s\n", __func__, __LINE__, hum_buf);
}

static void sys_screen_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_LOADED) {
        if (sys_update_timer == NULL) {
            sys_update_timer = lv_timer_create(sys_timer_cb, 1000, NULL);
        } else {
            lv_timer_resume(sys_update_timer);
        }
    } else if (code == LV_EVENT_SCREEN_UNLOADED) {
        if (sys_update_timer) {
            lv_timer_pause(sys_update_timer);
        }
    }
}

static void create_sys_status_page(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    pages[PAGE_SYS_STATUS] = scr;
    label_sys_info = lv_label_create(scr);
    lv_label_set_text(label_sys_info, " ");
    lv_obj_align(label_sys_info, LV_ALIGN_TOP_LEFT, 0, 0);

    label_battery_info = lv_label_create(scr);
    lv_label_set_text(label_battery_info, " ");
    lv_obj_align(label_battery_info, LV_ALIGN_TOP_LEFT, 0, 70);

    label_humiture_info = lv_label_create(scr);
    lv_label_set_text(label_humiture_info, " ");
    lv_obj_align(label_humiture_info, LV_ALIGN_TOP_LEFT, 0, 110);

    label_time_info = lv_label_create(scr);
    lv_label_set_text(label_time_info, " ");
    lv_obj_align(label_time_info, LV_ALIGN_TOP_LEFT, 0, 150);

    lv_obj_add_event_cb(scr, sys_screen_event_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(scr, sys_screen_event_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
}

static void create_sd_card_test_page(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    pages[PAGE_SD_CARD_TEST] = scr;
    label_sd_info = lv_label_create(scr);
    lv_obj_align(label_sd_info, LV_ALIGN_CENTER, 0, 0);
}

static void create_color_test_page(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    pages[PAGE_COLOR_TEST] = scr;
    color_title = lv_label_create(scr);
    lv_label_set_text(color_title, "Color Test");
    lv_obj_align(color_title, LV_ALIGN_TOP_MID, 0, 10);
    label_color_name = lv_label_create(scr);
    lv_obj_align(label_color_name, LV_ALIGN_CENTER, 0, 0);
}

static void create_audio_test_page(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    pages[PAGE_AUDIO_TEST] = scr;
    label_audio_info = lv_label_create(scr);
    lv_label_set_text(label_audio_info, "Audio Test\nShort press\nPowerKey\nreboot");
    lv_obj_align(label_audio_info, LV_ALIGN_CENTER, 0, 0);
}

static void create_power_off_page(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    page_power_off = scr;
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Power off\nPlease release\nPower Key");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

static void update_page_content(test_page_t page) {
    char buf[128];
    switch(page) {
        case PAGE_SD_CARD_TEST:

            sd_test_passed = false;
            sd_card_mounted = false;
    
            for (size_t i = 0; i < 2; i++) {

                sd_card_capacity = sd_card_get_size();
                sd_test_result_t result = sd_increment_counter_file("counter.txt");
                switch (result) {
                    case SD_TEST_OK:
                        printf("SD counter test passed.\n");
                        sd_test_passed = true;
                        sd_card_mounted = true;
                        break;
                    case SD_TEST_READ_FAILED:
                        printf("SD read failed - cannot retrieve current count.\n");
                        break;
                    case SD_TEST_WRITE_FAILED:
                        printf("SD write failed - cannot update counter.\n");
                        break;
                    default:
                        break;
                }

                if (sd_card_capacity > 0 && result == SD_TEST_OK) {
                    break;
                }
                
                DEV_Delay_ms(1000);
            }

            snprintf(buf, sizeof(buf),
                "SDcard Test\nMounted: %s\nCapacity: %luMB\nTest: %s",
                sd_card_mounted ? "Yes" : "No",
                sd_card_capacity,
                sd_test_passed ? "PASS" : "FAIL");
            lv_label_set_text(label_sd_info, buf);


            printf(" [%s:%d]%s\n", __func__, __LINE__, buf);
            break;

        case PAGE_COLOR_TEST: 
            static const char *names[] = {"Black","White"};
            static const lv_color_t colors[] = {
                LV_COLOR_MAKE(0,   0,   0),    // Black
                LV_COLOR_MAKE(255, 255, 255),  // White
            };
               
            static lv_style_t style_text = {0};
            lv_style_init(&style_text);

            if (lv_color_brightness(colors[color_index % 2]) > 128) {
                lv_style_set_text_color(&style_text, colors[0]);
            } else {
                lv_style_set_text_color(&style_text, colors[1]);
            }

            lv_obj_add_style(label_color_name, &style_text, LV_PART_MAIN);
            lv_obj_add_style(color_title, &style_text, LV_PART_MAIN);

            lv_label_set_text(label_color_name, names[color_index % 2]);
            lv_obj_set_style_bg_color(pages[PAGE_COLOR_TEST], colors[color_index % 2], LV_PART_MAIN);

            printf(" [%s:%d]%s\n", __func__, __LINE__, names[color_index % 2]);
            break;

        case PAGE_AUDIO_TEST:

            // lv_label_set_text(label_audio_info, "Audio Test\n reboot");

            // EPD_1IN54_V2_Init();
            // EPD_1IN54_V2_Clear();

            // watchdog_enable(1, true);

            break;

        case PAGE_TOUCH_ENTRY:


            break;

        default:
            break;
    }
}

static void global_key_event_cb(lv_event_t *e) {
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_key_t key = *((lv_key_t *)lv_event_get_param(e));

    printf(" [%s:%d]key:%d (0x%02X)\n", __func__, __LINE__, key, key);

    if (key == LV_KEY_POWER_LONG && current_page != PAGE_AUDIO_TEST) {
        lv_scr_load(page_power_off);
        lv_refr_now(NULL);
        DEV_Delay_ms(500);
        EPD_1IN54_V2_Init();
        EPD_1IN54_V2_Clear();
        DEV_Digital_Write(BAT_EN_PIN, 0);
        return;
    } else if(key == LV_KEY_ENTER && current_page == PAGE_AUDIO_TEST) {
        EPD_1IN54_V2_Init();
        EPD_1IN54_V2_Clear();
        watchdog_enable(1, true);
    }

    if(key == LV_KEY_LEFT) {
        current_page = (current_page == PAGE_SYS_STATUS) ? PAGE_MAX : (current_page - 1);

        if (current_page == PAGE_TOUCH_ENTRY && pages[current_page] == NULL) {
            create_touch_entry_page();
        }
        
        lv_scr_load(pages[current_page]);
        printf(" [%s:%d]current_page:%d\n", __func__, __LINE__, current_page);
        return;
    }

    if(key == LV_KEY_RIGHT) {
        current_page = (current_page == PAGE_MAX) ? PAGE_SYS_STATUS : (current_page + 1);

        if (current_page == PAGE_TOUCH_ENTRY && pages[current_page] == NULL) {
            create_touch_entry_page();
        }

        lv_scr_load(pages[current_page]);
        printf(" [%s:%d]current_page:%d\n", __func__, __LINE__, current_page);
        return;
    }

    if(key == LV_KEY_ENTER) {
        switch(current_page) {
            case PAGE_SD_CARD_TEST: sd_test_passed = !sd_test_passed; break;
            case PAGE_COLOR_TEST: color_index++; break;
            case PAGE_AUDIO_TEST: break;
            case PAGE_TOUCH_ENTRY: break;
            default: return;
        }
        update_page_content(current_page);
    }
}

static void lv_register_global_key_event(void) {
    static lv_group_t *g = NULL;
    if(g == NULL) {
        g = lv_group_create();
        for(int i = 1; i <= PAGE_MAX; i++) {
            lv_group_add_obj(g, pages[i]);
        }
        lv_indev_set_group(indev_keypad, g);
    }

    for(int i = 1; i <= PAGE_MAX; i++) {
        lv_obj_add_event_cb(pages[i], global_key_event_cb, LV_EVENT_KEY, NULL);
    }
}

void lv_app_test_init(void) {

    create_power_off_page();
    create_sys_status_page();
    create_sd_card_test_page();
    create_color_test_page();
    create_audio_test_page();
    create_touch_entry_page(); 

    update_page_content(PAGE_SD_CARD_TEST);
    update_page_content(PAGE_COLOR_TEST);
    // update_page_content(PAGE_AUDIO_TEST);

    lv_register_global_key_event();

    lv_scr_load(pages[PAGE_SYS_STATUS]);
    current_page = PAGE_SYS_STATUS;

    if (led_blink_timer == NULL) {
        led_blink_timer = lv_timer_create(led_blink_timer_cb, 500, NULL);
    }
}

#if INPUTDEV_TS

#define TEST_POINT_COUNT 4
#define TOUCH_THRESHOLD 50//30//20

typedef struct {
    uint16_t x;
    uint16_t y;
    bool touched;      
    uint16_t actual_x; 
    uint16_t actual_y; 
    float deviation;   
    lv_obj_t *checkbox;
} TestPoint;

typedef struct {
    TestPoint points[TEST_POINT_COUNT];
    int current_index;
    int total_tests;
    int passed_tests;
    bool is_testing;
    lv_obj_t *test_label; 
} TouchTestUI;

static TouchTestUI g_touch_test = {0};
static lv_obj_t *title = NULL;
static lv_obj_t *screen = NULL;    

void handle_touch_event(lv_event_t *e);
void return_to_main_screen(lv_event_t *e);

float calculate_deviation(int x1, int y1, int x2, int y2)
{
    return sqrtf(powf(x2 - x1, 2) + powf(y2 - y1, 2));
}


#if (TEST_POINT_COUNT == 4)

void init_test_points(void)
{
    uint16_t positions[TEST_POINT_COUNT][2] = {
        {20, 20},   // Top-Left
        {180, 20},  // Top-Right
        {20, 180},  // Bottom-Left
        {180, 180}  // Bottom-Right
    };

    for (int i = 0; i < TEST_POINT_COUNT; i++) {
        g_touch_test.points[i].x = positions[i][0];
        g_touch_test.points[i].y = positions[i][1];
        g_touch_test.points[i].touched = false;
        g_touch_test.points[i].actual_x = 0;
        g_touch_test.points[i].actual_y = 0;
        g_touch_test.points[i].deviation = 0.0f;
        g_touch_test.points[i].checkbox = NULL;
    }

    g_touch_test.current_index = 0;
    g_touch_test.total_tests = 0;
    g_touch_test.passed_tests = 0;
    g_touch_test.is_testing = true;
}

void create_touch_test_ui(void)
{
    screen = lv_scr_act();
    lv_obj_clean(screen);

    title = lv_label_create(screen);
    lv_label_set_text(title, "Touch Accuracy Test");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    g_touch_test.test_label = lv_label_create(screen);
    lv_obj_set_width(g_touch_test.test_label, 180);
    lv_obj_set_style_text_align(g_touch_test.test_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_touch_test.test_label, LV_ALIGN_CENTER, 0, 0);
    
    init_test_points();

    for (int i = 0; i < TEST_POINT_COUNT; i++) {
        lv_obj_t *chk = lv_checkbox_create(screen);
        lv_checkbox_set_text(chk, "");
        lv_obj_set_size(chk, 24, 24); 
        
        lv_obj_align_to(chk, screen, LV_ALIGN_TOP_LEFT, 
                        g_touch_test.points[i].x - 12, // -12 是为了让中心对准坐标点 (24/2)
                        g_touch_test.points[i].y - 12);
        
        g_touch_test.points[i].checkbox = chk;

        lv_obj_clear_flag(chk, LV_OBJ_FLAG_CLICKABLE); 
        lv_obj_add_flag(chk, LV_OBJ_FLAG_EVENT_BUBBLE);
        
        lv_obj_clear_state(chk, LV_STATE_CHECKED);
    }

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Please tap the 4 corners.\nProgress: 0/%d", TEST_POINT_COUNT);
    lv_label_set_text(g_touch_test.test_label, buffer);

    lv_obj_add_event_cb(screen, handle_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, return_to_main_screen, LV_EVENT_SHORT_CLICKED, NULL);
}

void return_to_main_screen(lv_event_t *e)
{
#if 0
    if (g_touch_test.current_index >= TEST_POINT_COUNT) {
        // 清理资源
        if (screen) {
            lv_obj_clean(screen);
            screen = NULL;
        }
        
        // 重置全局状态，以便下次进入时重新初始化
        memset(&g_touch_test, 0, sizeof(TouchTestUI));
        
        // 返回主测试菜单
        lv_app_test_init();
    }
#endif

    if (g_touch_test.current_index >= TEST_POINT_COUNT) {
        if (screen) {
            lv_obj_clean(screen);
            screen = NULL;
            pages[current_page] = NULL;
        }
        memset(&g_touch_test, 0, sizeof(TouchTestUI));
        
        current_page = (current_page == PAGE_SYS_STATUS) ? PAGE_MAX : (current_page - 1);
        lv_scr_load(pages[current_page]);
        printf("Touch test done. Switched to Page %d\n", current_page);
    }
}

void handle_touch_event(lv_event_t *e)
{
     if (LV_EVENT_PRESSED != lv_event_get_code(e)) {
        return;
    }

    if (!g_touch_test.is_testing) {
        return;
    }

    if (g_touch_test.current_index >= TEST_POINT_COUNT) {
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t point;
    lv_indev_get_point(indev, &point);

    int idx = g_touch_test.current_index;
    TestPoint *current = &g_touch_test.points[idx];

    current->actual_x = point.x;
    current->actual_y = point.y;
    
    current->deviation = calculate_deviation(current->x, current->y, point.x, point.y);

    printf("Touch point %d: Target (%d,%d) Actual (%d,%d) Deviation %.2f\n",
           idx + 1, current->x, current->y, point.x, point.y, current->deviation);

    bool is_pass = (current->deviation <= TOUCH_THRESHOLD);
    
    if (is_pass) {
        g_touch_test.passed_tests++;
        lv_obj_add_state(current->checkbox, LV_STATE_CHECKED);
    } else {
        printf("Deviation too large!\n");
    }

    g_touch_test.total_tests++;
    g_touch_test.current_index++;

    char buffer[128];
    if (g_touch_test.current_index < TEST_POINT_COUNT) {
        snprintf(buffer, sizeof(buffer), "Point %d OK.\nNext: Tap Corner %d/%d", 
                 idx + 1, g_touch_test.current_index + 1, TEST_POINT_COUNT);
        lv_label_set_text(g_touch_test.test_label, buffer);
    } else {
        float accuracy_rate = (float)g_touch_test.passed_tests / TEST_POINT_COUNT * 100;
        snprintf(buffer, sizeof(buffer), "Test Complete!\nResult: %s\nAccuracy: %.0f%%\n\nClick to Return", 
                 (g_touch_test.passed_tests == TEST_POINT_COUNT) ? "PASS" : "FAIL",
                 accuracy_rate);
        lv_label_set_text(g_touch_test.test_label, buffer);
    }
}

#elif (TEST_POINT_COUNT == 9)

void init_test_points(void)
{
    uint16_t positions[TEST_POINT_COUNT][2] = {

    #if 1
        {20, 20},  
        {100, 20}, 
        {180, 20}, 
        {20, 100}, 
        {100, 100},
        {180, 100},
        {20, 180}, 
        {100, 180},
        {180, 180} 
    #else
        {30, 30}, {100, 30}, {170, 30},
        {30, 100}, {100, 100}, {170, 100},
        {30, 170}, {100, 170}, {170, 170}
    #endif

    };

    for (int i = 0; i < TEST_POINT_COUNT; i++) {
        g_touch_test.points[i].x = positions[i][0];
        g_touch_test.points[i].y = positions[i][1];
        g_touch_test.points[i].touched = false;
        g_touch_test.points[i].actual_x = 0;
        g_touch_test.points[i].actual_y = 0;
        g_touch_test.points[i].deviation = 0.0f;
        g_touch_test.points[i].checkbox = NULL;
    }

    g_touch_test.current_index = 0;
    g_touch_test.total_tests = 0;
    g_touch_test.passed_tests = 0;
    g_touch_test.is_testing = true;
}

void create_touch_test_ui(void)
{
    screen = lv_scr_act();
    lv_obj_clean(screen);

    title = lv_label_create(screen);
    lv_label_set_text(title, "Touch Accuracy Test");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *desc = lv_label_create(screen);
    lv_label_set_text(desc, "Tap on the check boxes");
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
    lv_obj_align_to(desc, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    g_touch_test.test_label = lv_label_create(screen);
    lv_obj_set_width(g_touch_test.test_label, 180);
    lv_label_set_text(g_touch_test.test_label, "");
    lv_obj_align(g_touch_test.test_label, LV_ALIGN_CENTER, 10, 40);

    init_test_points();

    for (int i = 0; i < TEST_POINT_COUNT; i++) {
        lv_obj_t *chk = lv_checkbox_create(screen);
        lv_checkbox_set_text(chk, "");
        lv_obj_set_size(chk, 20, 20); 
        lv_obj_align_to(chk, screen, LV_ALIGN_TOP_LEFT, g_touch_test.points[i].x - 10, g_touch_test.points[i].y - 10);
        g_touch_test.points[i].checkbox = chk;

        lv_obj_clear_flag(chk, LV_OBJ_FLAG_CLICKABLE); 
        lv_obj_add_flag(chk, LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    char buffer[200];
    snprintf(buffer, sizeof(buffer), "Tap #%d/%d - \nTarget: (%d, %d)", 
             g_touch_test.current_index + 1, TEST_POINT_COUNT,
             g_touch_test.points[0].x, g_touch_test.points[0].y);
    lv_label_set_text(g_touch_test.test_label, buffer);
    
    lv_obj_add_state(g_touch_test.points[0].checkbox, LV_STATE_CHECKED);

    lv_obj_add_event_cb(screen, handle_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, return_to_main_screen, LV_EVENT_SHORT_CLICKED, NULL);
}


void return_to_main_screen(lv_event_t *e)
{
    if (g_touch_test.current_index >= TEST_POINT_COUNT) {
        lv_obj_clean(screen);
        screen = NULL;

        lv_app_test_init();
    }
}

void handle_touch_event(lv_event_t *e)
{
    if ( LV_EVENT_PRESSED != lv_event_get_code(e) ) {
        return;
    }

    if (!g_touch_test.is_testing || g_touch_test.current_index >= TEST_POINT_COUNT) {
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t point;
    lv_indev_get_point(indev, &point);

    TestPoint *current = &g_touch_test.points[g_touch_test.current_index];
    current->touched = true;
    current->actual_x = point.x;
    current->actual_y = point.y;
    current->deviation = calculate_deviation(current->x, current->y, point.x, point.y);

    if (current->deviation <= TOUCH_THRESHOLD) {
        g_touch_test.passed_tests++;
    }

    g_touch_test.total_tests++;

    printf("Touch point %d: Target (%d,%d) Actual (%d,%d) Deviation %.2f pixels\n",
           g_touch_test.current_index + 1,
           current->x, current->y,
           point.x, point.y,
           current->deviation);

    lv_obj_clear_state(current->checkbox, LV_STATE_CHECKED);
    
    g_touch_test.current_index++;

    if (g_touch_test.current_index < TEST_POINT_COUNT) {
        lv_obj_add_state(g_touch_test.points[g_touch_test.current_index].checkbox, LV_STATE_CHECKED);

        char buffer[200];
        snprintf(buffer, sizeof(buffer), "Tap #%d/%d - \nTarget: (%d, %d)", 
                 g_touch_test.current_index + 1, TEST_POINT_COUNT,
                 g_touch_test.points[g_touch_test.current_index].x, 
                 g_touch_test.points[g_touch_test.current_index].y);
        lv_label_set_text(g_touch_test.test_label, buffer);
    } else {
        float accuracy_rate = (float)g_touch_test.passed_tests / TEST_POINT_COUNT * 100;
        char buffer[300];
        snprintf(buffer, sizeof(buffer), "Test Complete!\nAccuracy: %d/%d (%.2f%%)\nTap anywhere to return", 
                 g_touch_test.passed_tests, TEST_POINT_COUNT, accuracy_rate);
        lv_label_set_text(g_touch_test.test_label, buffer);
    }
}

#else

#endif


#endif


