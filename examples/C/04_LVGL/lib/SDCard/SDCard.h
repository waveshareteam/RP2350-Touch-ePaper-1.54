#ifndef __SPI_CARD_
#define __SPI_CARD_


#include <stdio.h>
#include "pico/stdlib.h"
#include <stdbool.h>
#include "hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUPPORT_SPI_SDCARD
// #define SUPPORT_SDIO_SDCARD

#ifdef SUPPORT_SPI_SDCARD
#define SPI_CARD_PORT   spi0
#define SPI_CARD_CLK    (18)
#define SPI_CARD_MOSI   (19) 
#define SPI_CARD_MISO   (20)
#define SPI_CARD_CS     (23)
#endif


#ifdef SUPPORT_SDIO_SDCARD
#define SDIO_CARD_CMD   (19)
#define SDIO_CARD_D0    (20)
#endif


#if (defined SUPPORT_SPI_SDCARD) || (defined SUPPORT_SDIO_SDCARD)

typedef enum {
    SD_TEST_OK = 0,
    SD_TEST_READ_FAILED,
    SD_TEST_WRITE_FAILED,
} sd_test_result_t;

sd_test_result_t sd_increment_counter_file(const char* filename);


bool sd_card_init(void);
void sd_card_deinit(void);

void sd_file_append_text(const char* const filename);
void sd_file_read_to_console(const char* const filename);
uint32_t sd_card_get_size(void);

#endif

#ifdef __cplusplus
}
#endif

#endif


