/* 接腳定義。
 *
 * 權威來源是 rp2040-retro-handheld/docs/HARDWARE.md —— 改動先改那裡。
 * 這份只列編輯器實際用到的：顯示器、SD、鍵盤矩陣。
 *
 * 用不到而刻意省略的：音效(GPIO 7)、8 鍵遊戲按鍵(2/3/4/5/6/8/9/28)。
 * 編輯器走鍵盤矩陣那一套，不是 D-pad。
 */
#ifndef BOARD_H
#define BOARD_H

/* ---- 顯示器 ILI9341 (spi0) ---- */
#define LCD_SPI          spi0
#define LCD_PIN_DC       20
#define LCD_PIN_CS       17
#define LCD_PIN_CLK      18
#define LCD_PIN_MOSI     19
#define LCD_PIN_RST      21
#define LCD_PIN_BL       22
#define LCD_SPI_HZ       (32 * 1000 * 1000)

#define LCD_W            320
#define LCD_H            240

/* ---- SD 卡 (spi1) ---- */
#define SD_SPI           spi1
#define SD_PIN_CS        13
#define SD_PIN_SCK       10
#define SD_PIN_MOSI      11
#define SD_PIN_MISO      12
#define SD_SPI_SLOW_HZ   (100 * 1000)
#define SD_SPI_FAST_HZ   (25 * 1000 * 1000)

/* ---- 8x8 鍵盤矩陣 (74HC595 掃描列 / 74HC165 讀回) ---- */
#define MTX_PIN_DATA_OUT 15      /* -> 595 */
#define MTX_PIN_LATCH    14      /* 595 latch,同時當 165 的 load */
#define MTX_PIN_CLOCK    26      /* 595 與 165 共用 */
#define MTX_PIN_DATA_IN  27      /* <- 165 */

#endif /* BOARD_H */
