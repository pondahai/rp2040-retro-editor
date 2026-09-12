/* 接腳定義。
 *
 * 權威來源是 rp2040-retro-handheld/docs/HARDWARE.md —— 改動先改那裡。
 * 這份只列編輯器實際用到的：顯示器、SD、鍵盤矩陣。
 *
 * 這台機器**兩套輸入並存**(見 HARDWARE.md):8x8 鍵盤矩陣,以及 8 顆直接接
 * GPIO 的遊戲按鍵。編輯器兩套都用 ——
 *
 *   矩陣    打字、注音
 *   D-pad   方向鍵。**鍵盤硬體已經把方向鍵拿掉了**,矩陣上那幾格現在空著,
 *           所以游標移動與候選字瀏覽改由 D-pad 負責。
 *
 * 用不到而刻意省略的:音效(GPIO 7)。
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

/* ---- 8 鍵遊戲按鍵:全部 active-low,用內部上拉 ---- */
#define BTN_PIN_UP       9
#define BTN_PIN_DOWN     5
#define BTN_PIN_LEFT     8
#define BTN_PIN_RIGHT    6
#define BTN_PIN_SELECT   28
#define BTN_PIN_START    4
#define BTN_PIN_A        2
#define BTN_PIN_B        3

#endif /* BOARD_H */
