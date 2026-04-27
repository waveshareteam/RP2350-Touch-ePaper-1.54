/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "DEV_Config.h"
#include "lv_port_disp.h"
#include <stdbool.h>
#include "EPD_1in54_V2.h"
#include "GUI_Paint.h"
#include <stdlib.h> // malloc() free()

#include "lvgl_display_config.h"


#define MY_DISP_HOR_RES (EPD_1IN54_V2_WIDTH)
#define MY_DISP_VER_RES (EPD_1IN54_V2_HEIGHT)

static lv_disp_drv_t disp_drv = {0};    /*Descriptor of a display driver*/



#if (defined EPAPER_DISPALY_FULL)

static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    UBYTE *BlackImage = (UBYTE *)drv->user_data;

  	uint16_t *buffer = (uint16_t *)color_p;

    EPD_1IN54_V2_Clear();

  	for(int y = area->y1; y <= area->y2; y++) 
  	{
  	 	for(int x = area->x1; x <= area->x2; x++)
  	 	{
  	 	   	uint8_t color = (*buffer < 0x7fff) ? BLACK : WHITE;
            Paint_SetPixel(x,y, color);
  	 	   	buffer++;
  	 	}
  	}

    EPD_1IN54_V2_Display(BlackImage);

	lv_disp_flush_ready(drv);

}

#elif (defined EPAPER_DISPALY_PART)

static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    UBYTE *BlackImage = (UBYTE *)drv->user_data;
  	uint16_t *buffer = (uint16_t *)color_p;

  	for(int y = area->y1; y <= area->y2; y++) 
  	{
  	 	for(int x = area->x1; x <= area->x2; x++)
  	 	{
  	 	   	uint8_t color = (*buffer < 0x7fff) ? BLACK : WHITE;
            Paint_SetPixel(x,y, color);
  	 	   	buffer++;
  	 	}
  	}

    EPD_1IN54_V2_DisplayPart(BlackImage);
	  lv_disp_flush_ready(drv);
}

#else

#endif

void lv_port_disp_init(void)
{
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_Clear();

#ifdef EPAPER_DISPALY_PART
	EPD_1IN54_V2_Init_Partial();
#endif

    static UBYTE img[(((EPD_1IN54_V2_WIDTH % 8 == 0)? (EPD_1IN54_V2_WIDTH / 8 ): (EPD_1IN54_V2_WIDTH / 8 + 1)) * EPD_1IN54_V2_HEIGHT)] = {0};
    UBYTE *BlackImage = img;
    Paint_NewImage(BlackImage, EPD_1IN54_V2_WIDTH, EPD_1IN54_V2_HEIGHT, ROTATE_0, WHITE);

    static lv_disp_draw_buf_t draw_buf_dsc_1 = {0};
    static lv_color_t buf_1[MY_DISP_HOR_RES * MY_DISP_VER_RES] = {0};
    static lv_color_t buf_2[MY_DISP_HOR_RES * MY_DISP_VER_RES] = {0};
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, buf_2, MY_DISP_HOR_RES * MY_DISP_VER_RES);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;
    disp_drv.user_data = BlackImage;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
#if 1 
    disp_drv.full_refresh = 1;
#endif 
    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}


#endif
