#include "hw_keys.h"
#include "board.h"
#include "pico/stdlib.h"
#if KBD_CLK_ON_LCD_DC
#include "hardware/spi.h"
#endif

/* 掃描時序原封搬自 PicoApple2.ino 的 scan_matrix()
 * (經由 PicoApple2-KeyboardTester/KeyboardTester.ino:122-135 確認)。
 *
 * 這段時序有兩個地方看起來像筆誤,但都是對的,不要「順手修掉」:
 *
 *   1. 送兩個 byte 給 595 —— 板子上是兩顆串聯,第一個 byte 會被推到
 *      更遠那顆去,真正選列的是第二個。
 *   2. LATCH 腳一支兼兩職 —— 595 的 latch 與 165 的 load。所以推完列選
 *      之後那串 1/0/1 的抖動是在「鎖存輸出、順便叫 165 取樣」。
 */

/* KBD_SHARED_DATA:GPIO 15 兼當 595 SER 與 165 QH(經 1 kΩ)。
 * 每次寫 595 前切成輸出、每次讀 165 前切成輸入。平常(掃描之外)停在輸入,
 * 讓 QH 不必一直跟 RP2040 的輸出對打。 */
static inline void data_pin_to_out(void)
{
#if KBD_SHARED_DATA
    gpio_set_dir(MTX_PIN_DATA_OUT, GPIO_OUT);
#endif
}

static inline void data_pin_to_in(void)
{
#if KBD_SHARED_DATA
    gpio_set_dir(MTX_PIN_DATA_IN, GPIO_IN);
    sleep_us(1);    /* 等 QH 經 1 kΩ 把腳位拉到位 */
#endif
}

static void shift_out(uint8_t v)
{
    /* MSB first */
    for (int i = 7; i >= 0; i--) {
        gpio_put(MTX_PIN_DATA_OUT, (v >> i) & 1);
        gpio_put(MTX_PIN_CLOCK, 1);
        sleep_us(1);
        gpio_put(MTX_PIN_CLOCK, 0);
    }
}

static uint8_t shift_in(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (gpio_get(MTX_PIN_DATA_IN)) data |= (uint8_t)(1u << i);
        gpio_put(MTX_PIN_CLOCK, 1);
        sleep_us(1);
        gpio_put(MTX_PIN_CLOCK, 0);
    }
    return data;
}

void hw_keys_init(void)
{
#if KBD_CLK_ON_LCD_DC
    /* CLOCK = LCD D/C,lcd_init() 已經初始化過,這裡不能再 gpio_init
     * (會把 D/C 拉回 0)。驅動電流/斜率維持預設,減少輻射。 */
    const uint pins_out[] = { MTX_PIN_LATCH };
#elif KBD_SHARED_DATA
    const uint pins_out[] = { MTX_PIN_LATCH, MTX_PIN_CLOCK };
#else
    const uint pins_out[] = { MTX_PIN_DATA_OUT, MTX_PIN_LATCH, MTX_PIN_CLOCK };
#endif
    for (unsigned i = 0; i < count_of(pins_out); i++) {
        gpio_init(pins_out[i]);
        gpio_set_dir(pins_out[i], GPIO_OUT);
        gpio_put(pins_out[i], 0);
    }
    gpio_init(MTX_PIN_DATA_IN);
    gpio_set_dir(MTX_PIN_DATA_IN, GPIO_IN);
#if KBD_SHARED_DATA
    /* 預設下拉約 50 kΩ 會跟 1 kΩ 分壓,關掉。
     * 先把輸出暫存器設 0,之後切成輸出時才不會冒出殘留的 1。 */
    gpio_disable_pulls(MTX_PIN_DATA_IN);
    gpio_put(MTX_PIN_DATA_OUT, 0);

    /* 27 已割線、留給 ADC:輸入、不上下拉,不要再當輸出 */
    gpio_init(MTX_PIN_SPARE);
    gpio_set_dir(MTX_PIN_SPARE, GPIO_IN);
    gpio_disable_pulls(MTX_PIN_SPARE);
#endif
#if KBD_CLK_ON_LCD_DC
    /* 26 也跟鍵盤無關了:高阻抗 */
    gpio_init(MTX_PIN_SPARE2);
    gpio_set_dir(MTX_PIN_SPARE2, GPIO_IN);
    gpio_disable_pulls(MTX_PIN_SPARE2);
    /* LATCH 閒置停在高:LCD 翻 D/C 時 595 輸出不動 */
    gpio_put(MTX_PIN_LATCH, 1);
#endif
}

static void scan_matrix(uint8_t rows[8])
{
#if KBD_CLK_ON_LCD_DC
    /* CLOCK 就是 LCD D/C:LCD 不能正在傳輸。LCD 驅動全走
     * spi_write_blocking 且跟這裡同在主迴圈,照理已經做完;仍再等一次
     * SPI 閒置、拉高 CS 保險。若日後改用 DMA 或另一核心畫圖,這裡要加同步。
     * D/C 電位記下來,掃完還原,LCD 驅動的狀態維持不變。 */
    while (spi_is_busy(LCD_SPI)) tight_loop_contents();
    gpio_put(LCD_PIN_CS, 1);
    bool dc_saved = gpio_get_out_level(LCD_PIN_DC);
    gpio_put(MTX_PIN_CLOCK, 0);
#endif
    for (int row = 0; row < 8; row++) {
        gpio_put(MTX_PIN_LATCH, 0);
        data_pin_to_out();
        shift_out(0);                          /* 給串聯的另一顆 595 */
        shift_out((uint8_t)(1u << row));       /* 選這一列 */
        gpio_put(MTX_PIN_LATCH, 1);
        sleep_us(5);
        gpio_put(MTX_PIN_LATCH, 0);
        sleep_us(1);
        gpio_put(MTX_PIN_LATCH, 1);

        data_pin_to_in();
        uint8_t col_data = shift_in();

        /* 165 讀回來的 bit 順序跟 col 相反:
         * 上游寫的是 keyState[row][7 - col] = col_data & (1 << col)。
         * vendor/keys.c 要的是 rows[r] 的 bit c = 1 表示 [r][c] 按住,
         * 所以這裡把整個 byte 反轉過來。 */
        uint8_t r = 0;
        for (int c = 0; c < 8; c++)
            if (col_data & (1u << c)) r |= (uint8_t)(1u << (7 - c));
        rows[row] = r;
    }

    /* 收尾的一拍,同樣照搬上游。
     * KBD_SHARED_DATA 時 GPIO 15 此刻已是輸入(最後一步是 shift_in),
     * 掃描結束就停在輸入。這一拍推進 595 的垃圾無害:下一輪每列都先
     * 重新寫滿 16 bit 才拉 LATCH。 */
    gpio_put(MTX_PIN_CLOCK, 1);
    sleep_us(2);
    gpio_put(MTX_PIN_CLOCK, 0);
#if KBD_CLK_ON_LCD_DC
    gpio_put(LCD_PIN_DC, dc_saved);
#endif
}

int hw_keys_poll(keys *k, key_event *out, int max)
{
    uint8_t rows[8];
    scan_matrix(rows);
    return keys_update(k, rows, to_ms_since_boot(get_absolute_time()), out, max);
}
