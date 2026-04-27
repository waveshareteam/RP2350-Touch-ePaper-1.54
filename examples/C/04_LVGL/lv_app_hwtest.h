#ifndef LV_APP_HWTEST_H
#define LV_APP_HWTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#include "lv_port_indev.h"

/*********************
 *      FUNCTION PROTOTYPES
 *********************/

extern int32_t *mic_sample_buffers;
extern int mic_num_samples;

void lv_app_test_init(void);

#if INPUTDEV_TS
void create_touch_test_ui(void);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_APP_HWTEST_H */
