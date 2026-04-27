
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include <stdlib.h> // malloc() free()
#include "EPD_1in54_V2.h"

void setup() {

    if(DEV_Module_Init()!=0){
        return;
    }
    Serial.printf("EPD_1in54_V2_test Demo\r\n");

    Serial.printf("e-Paper Init and Clear...\r\n");
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_Clear();
    DEV_Delay_ms(500);

    //Create a new image cache
    UBYTE *BlackImage;
    UWORD Imagesize = ((EPD_1IN54_V2_WIDTH % 8 == 0)? (EPD_1IN54_V2_WIDTH / 8 ): (EPD_1IN54_V2_WIDTH / 8 + 1)) * EPD_1IN54_V2_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        Serial.printf("Failed to apply for black memory...\r\n");
        return;
    }
    Serial.printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_1IN54_V2_WIDTH, EPD_1IN54_V2_HEIGHT, ROTATE_0, WHITE);
    
#if 1   //show image for array    
    Serial.printf("show image for array\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(gImage_1in54);

    EPD_1IN54_V2_Display(BlackImage);
    DEV_Delay_ms(5000);
#endif

#if 1   // Drawing on the image
    Serial.printf("Drawing\r\n");
    //1.Select Image
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // 2.Drawing on the image
    Paint_DrawPoint(5, 10, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(5, 25, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(5, 40, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawPoint(5, 55, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT);

    Paint_DrawLine(20, 10, 70, 60, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 10, 20, 60, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(170, 15, 170, 55, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(150, 35, 190, 35, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawRectangle(20, 10, 70, 60, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(85, 10, 130, 60, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawCircle(170, 35, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(170, 85, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(5, 85, "waveshare", &Font20, BLACK, WHITE);
    Paint_DrawNum(5, 110, 123456789, &Font20, BLACK, WHITE);

    Paint_DrawString_CN(5, 130,"你好Abc", &Font12CN, BLACK, WHITE);
    Paint_DrawString_CN(5, 155, "微雪Abc", &Font24CN, WHITE, BLACK);

    EPD_1IN54_V2_Display(BlackImage);
	
    DEV_Delay_ms(2000);
#endif

#if 1   //Partial refresh, example shows time    
    // The image of the previous frame must be uploaded, otherwise the
    // first few seconds will display an exception.
	EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
	
	// enter partial mode
	EPD_1IN54_V2_Init_Partial();
    Serial.printf("Partial refresh\r\n");
    Paint_SelectImage(BlackImage);
    PAINT_TIME sPaint_time;
    sPaint_time.Hour = 12;
    sPaint_time.Min = 34;
    sPaint_time.Sec = 56;
    UBYTE num = 15;
    for (;;) {
        sPaint_time.Sec = sPaint_time.Sec + 1;
        if (sPaint_time.Sec == 60) {
            sPaint_time.Min = sPaint_time.Min + 1;
            sPaint_time.Sec = 0;
            if (sPaint_time.Min == 60) {
                sPaint_time.Hour =  sPaint_time.Hour + 1;
                sPaint_time.Min = 0;
                if (sPaint_time.Hour == 24) {
                    sPaint_time.Hour = 0;
                    sPaint_time.Min = 0;
                    sPaint_time.Sec = 0;
                }
            }
        }
        Paint_ClearWindows(15, 65, 15 + Font20.Width * 7, 65 + Font20.Height, WHITE);
        Paint_DrawTime(15, 65, &sPaint_time, &Font20, WHITE, BLACK);
        num = num - 1;
        if(num == 0) {
            break;
        }
        EPD_1IN54_V2_DisplayPart(BlackImage);
        DEV_Delay_ms(500);//Analog clock 1s
    }

#endif

    Serial.printf("Clear...\r\n");
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_Clear();

    Serial.printf("Goto Sleep...\r\n");
    EPD_1IN54_V2_Sleep();
    free(BlackImage);
    BlackImage = NULL;
    DEV_Delay_ms(2000);//important, at least 2s
    // close 5V
    Serial.printf("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}

void loop() {
  Serial.printf("running \r\n");
  DEV_Delay_ms(2000);
}
