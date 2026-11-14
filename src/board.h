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

/* KBD_CLK_ON_LCD_DC = 1:595/165 的 CLOCK 改接 LCD 的 D/C(GPIO 20)。
 * LCD 只在 CS 低時於 SCK 邊緣取樣 D/C,所以掃描時只要 LCD 沒在傳輸、
 * CS 為高,D/C 怎麼跳都無妨;平常 LCD 翻 D/C 只會讓 595/165 多移幾位,
 * LATCH 不動 595 輸出就不變,165 每次掃描開頭也會重新載入。
 * 必須同時開 KBD_SHARED_DATA。GPIO 26、27 都空出來。 */
/* 預設 1 = 現行接線(rp2040-retro-handheld docs/HARDWARE.md §2.2)。 */
#ifndef KBD_SHARED_DATA
#define KBD_SHARED_DATA  1
#endif
#ifndef KBD_CLK_ON_LCD_DC
#define KBD_CLK_ON_LCD_DC 1
#endif

#if KBD_CLK_ON_LCD_DC
#if !KBD_SHARED_DATA
#error "KBD_CLK_ON_LCD_DC requires KBD_SHARED_DATA"
#endif
#define MTX_PIN_CLOCK    LCD_PIN_DC  /* 595 與 165 共用,兼 LCD D/C */
#define MTX_PIN_SPARE2   26          /* 空出,高阻抗 */
#else
#define MTX_PIN_CLOCK    26      /* 595 與 165 共用 */
#endif

/* KBD_SHARED_DATA = 1:硬體改過的機器 —— 165 的 QH 不再接 GPIO 27,改成串
 * 1 kΩ 接到 GPIO 15,跟 595 的 SER 共用一支腳。寫 595 時 GPIO 15 當輸出
 * (隔著 1 kΩ 蓋過 QH),讀 165 時切成輸入。GPIO 27 空出來留給 ADC。
 * 設 0 = 舊接線(QH 在 GPIO 27)。預設值定義在上方。
 * 舊機器用 cmake -DKBD_SHARED_DATA=OFF -DKBD_CLK_ON_LCD_DC=OFF。 */

#if KBD_SHARED_DATA
#define MTX_PIN_DATA_IN  15      /* <- 165 QH,經 1 kΩ,與 DATA_OUT 共用 */
#define MTX_PIN_SPARE    27      /* 割線後空出,留給 ADC */
#if KBD_CLK_ON_LCD_DC
#define KBD_MODE_STR     "kbd QH GP15 CLK DC"
#else
#define KBD_MODE_STR     "kbd QH on GP15"
#endif
#else
#define MTX_PIN_DATA_IN  27      /* <- 165 */
#define KBD_MODE_STR     "kbd QH on GP27"
#endif

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
