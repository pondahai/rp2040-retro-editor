#include "hw_keys.h"
#include "board.h"
#include "pico/stdlib.h"

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
    const uint pins_out[] = { MTX_PIN_DATA_OUT, MTX_PIN_LATCH, MTX_PIN_CLOCK };
    for (unsigned i = 0; i < count_of(pins_out); i++) {
        gpio_init(pins_out[i]);
        gpio_set_dir(pins_out[i], GPIO_OUT);
        gpio_put(pins_out[i], 0);
    }
    gpio_init(MTX_PIN_DATA_IN);
    gpio_set_dir(MTX_PIN_DATA_IN, GPIO_IN);
}

static void scan_matrix(uint8_t rows[8])
{
    for (int row = 0; row < 8; row++) {
        gpio_put(MTX_PIN_LATCH, 0);
        shift_out(0);                          /* 給串聯的另一顆 595 */
        shift_out((uint8_t)(1u << row));       /* 選這一列 */
        gpio_put(MTX_PIN_LATCH, 1);
        sleep_us(5);
        gpio_put(MTX_PIN_LATCH, 0);
        sleep_us(1);
        gpio_put(MTX_PIN_LATCH, 1);

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

    /* 收尾的一拍,同樣照搬上游 */
    gpio_put(MTX_PIN_CLOCK, 1);
    sleep_us(2);
    gpio_put(MTX_PIN_CLOCK, 0);
}

int hw_keys_poll(keys *k, key_event *out, int max)
{
    uint8_t rows[8];
    scan_matrix(rows);
    return keys_update(k, rows, to_ms_since_boot(get_absolute_time()), out, max);
}
