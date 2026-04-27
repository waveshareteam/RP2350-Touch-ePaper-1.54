#include "SDCard.h"

#if (defined SUPPORT_SPI_SDCARD) || (defined SUPPORT_SDIO_SDCARD)

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"
#include <string.h>
#include <stdlib.h>

/*

This file should be tailored to match the hardware design.

See
https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

*/

#ifdef SUPPORT_SPI_SDCARD

/* SPI Interface */
// Hardware Configuration of SPI "object"
static spi_t spi  = {  // One for each RP2040 SPI component used
    .hw_inst = SPI_CARD_PORT,  // SPI component
    .sck_gpio = SPI_CARD_CLK,  // GPIO number (not Pico pin number)
    .mosi_gpio = SPI_CARD_MOSI,
    .miso_gpio = SPI_CARD_MISO,
    .set_drive_strength = true,
    // .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    // .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
    .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .no_miso_gpio_pull_up = true,
    // .spi_mode = 3,
    .baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
    //.baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
    // .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
    //.baud_rate = 125 * 1000 * 1000 / 2  // 62500000 Hz
};

/* SPI Interface */
static sd_spi_if_t spi_if = {
        .spi = &spi,  // Pointer to the SPI driving this card
        .ss_gpio = SPI_CARD_CS,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        // .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA
};

// Hardware Configuration of the SD Card "object"
static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card

    // SD Card detect:
    .use_card_detect = false,
    .card_detect_gpio = 43,  
    .card_detected_true = 0, // What the GPIO read returns when a card is present.
    .card_detect_use_pull = true,
    .card_detect_pull_hi = true   
};

#elif (defined SUPPORT_SDIO_SDCARD)

/* SDIO Interface */
static sd_sdio_if_t sdio_if = {
    /*
    Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
    The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
        CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
        As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
            which is -2 in mod32 arithmetic, so:
        CLK_gpio = D0_gpio -2.
        D1_gpio = D0_gpio + 1;
        D2_gpio = D0_gpio + 2;
        D3_gpio = D0_gpio + 3;
    */
    .CMD_gpio = SDIO_CARD_CMD,
    .D0_gpio = SDIO_CARD_D0,
    .CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .SDIO_PIO = pio1,
    .DMA_IRQ_num = DMA_IRQ_1,
    // .baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
    .baud_rate = 125 * 1000 * 1000 / 7  // 17857143 Hz
    // .baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
    // .baud_rate = 125 * 1000 * 1000 / 5  // 25000000 Hz
    // .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
};

/* Hardware Configuration of the SD Card socket "object" */
static sd_card_t sd_card = {
    .type = SD_IF_SDIO, 
    .sdio_if_p = &sdio_if,
    // SD Card detect:
    .use_card_detect = false,
    .card_detect_gpio = 24,  
    .card_detected_true = 0, // What the GPIO read returns when a card is
                                // present.
    .card_detect_use_pull = true,
    .card_detect_pull_hi = true
};

#endif


/**
 * @brief Get the number of SD cards.
 *
 * @return The number of SD cards, which is 1 in this case.
 */
size_t sd_get_num() { return 1; }

/**
 * @brief Get a pointer to an SD card object by its number.
 *
 * @param[in] num The number of the SD card to get.
 *
 * @return A pointer to the SD card object, or @c NULL if the number is invalid.
 */
sd_card_t* sd_get_by_num(size_t num) {
    if (0 == num) {
        // The number 0 is a valid SD card number.
        // Return a pointer to the sd_card object.
        return &sd_card;
    } else {
        // The number is invalid. Return @c NULL.
        return NULL;
    }
}


bool sd_card_init(void)
{
    static FATFS fs;
    FRESULT fr = f_mount(&fs, "0:", 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }

    printf("f_mount succeed: %s (%d)\n", FRESULT_str(fr), fr);
    return true;
}

void sd_card_deinit(void)
{
    f_unmount("");
}

void sd_file_append_text(const char* const filename)
{
    FIL fil;
    FRESULT fr;

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return;
    }

    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }

    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
}

void sd_file_read_to_console(const char* const filename)
{
    FIL fil = {0};            
    FRESULT fr = FR_OK;         
    char read_buf[1024] = {0};    
    UINT bytes_read = 0;    

    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("f_open(%s) failed: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return;
    }

    printf("===== start read file: %s =====\n", filename);

    while (1) {
        fr = f_read(&fil, read_buf, sizeof(read_buf) - 1, &bytes_read);
        if (fr != FR_OK) {
            printf("f_read failed: %s (%d)\n", FRESULT_str(fr), fr);
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        read_buf[bytes_read] = '\0';

        printf("read %u byte: %s", bytes_read, read_buf);
    }

    printf("===== end read file: %s =====\n", filename);

    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("f_close failed: %s (%d)\n", FRESULT_str(fr), fr);
    }
}


uint32_t sd_card_get_size(void)
{
    const char *arg = "0";
    sd_card_t *sd_card_p = sd_get_by_drive_prefix(arg);
    FATFS *fs_p = &sd_card_p->state.fatfs;

    DWORD fre_clust, fre_sect, tot_sect;
    FRESULT fr = f_getfree(arg, &fre_clust, &fs_p);
    if (FR_OK != fr) {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }
    /* Get total sectors and free sectors */
    tot_sect = (fs_p->n_fatent - 2) * fs_p->csize;
    fre_sect = fre_clust * fs_p->csize;
    /* Print the free space (assuming 512 bytes/sector) */
    printf("\n%10lu KiB (%lu MiB) total drive space.\n%10lu KiB (%lu MiB) available.\n",
           tot_sect / 2, tot_sect / 2 / 1024,
           fre_sect / 2, fre_sect / 2 / 1024);
    
    return tot_sect / 2 / 1024;
}




static int32_t parse_last_number_from_file(FIL* fil)
{
    char line[32];
    int32_t last_num = 0;
    FRESULT fr;
    UINT bytes_read;

    printf("[SD Debug] File raw content:\n");
    f_lseek(fil, 0); 

    char ch;
    while (1) {
        fr = f_read(fil, &ch, 1, &bytes_read);
        if (fr != FR_OK || bytes_read == 0) break;
        putchar(ch); 
    }
    printf("[SD Debug] End of file\n");

    f_lseek(fil, 0);

    while (1) {
        char* buf = line;
        size_t pos = 0;
        while (pos < sizeof(line) - 1) {
            fr = f_read(fil, &buf[pos], 1, &bytes_read);
            if (fr != FR_OK || bytes_read == 0) break;
            if (buf[pos] == '\n' || buf[pos] == '\r') {
                buf[pos] = '\0';
                break;
            }
            pos++;
        }
        if (pos == 0 && bytes_read == 0) break; // EOF

        if (pos == 0) continue;

        char* endptr;
        long val = strtol(line, &endptr, 10);
        if (*endptr == '\0' && val > 0 && val <= INT32_MAX) {
            last_num = (int32_t)val;
        }
    }
    return last_num;
}

sd_test_result_t sd_increment_counter_file(const char* filename)
{
    FIL fil;
    FRESULT fr;
    int32_t current_value = 0;

    fr = f_open(&fil, filename, FA_READ);
    if (fr == FR_OK) {
        current_value = parse_last_number_from_file(&fil);
        f_close(&fil);
    } else if (fr == FR_NO_FILE) {
        current_value = 0;
    } else {
        printf("f_open(%s) for read failed: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return SD_TEST_READ_FAILED;
    }

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK) {
        if (fr == FR_NO_FILE) {
            fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
        }
        if (fr != FR_OK) {
            printf("f_open(%s) for write failed: %s (%d)\n", filename, FRESULT_str(fr), fr);
            return SD_TEST_WRITE_FAILED;
        }
    }

    int next_value = current_value + 1;
    char write_buf[32];
    int len = snprintf(write_buf, sizeof(write_buf), "%d\n", next_value);
    
    UINT bytes_written;
    fr = f_write(&fil, write_buf, len, &bytes_written);
    if (fr != FR_OK || bytes_written != (UINT)len) {
        printf("f_write failed: %s (%d), wrote %u/%d bytes\n", 
               FRESULT_str(fr), fr, bytes_written, len);
        f_close(&fil);
        return SD_TEST_WRITE_FAILED;
    }

    fr = f_sync(&fil);
    if (fr != FR_OK) {
        printf("f_sync failed: %s (%d)\n", FRESULT_str(fr), fr);
        f_close(&fil);
        return SD_TEST_WRITE_FAILED;
    }

    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("f_close after write failed: %s (%d)\n", FRESULT_str(fr), fr);
        return SD_TEST_WRITE_FAILED;
    }

    printf("SD counter file '%s': wrote %d (previous was %d)\n", 
           filename, next_value, current_value);
    return SD_TEST_OK;
}


#endif
