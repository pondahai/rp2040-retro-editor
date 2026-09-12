#include "hw_dpad.h"
#include "board.h"
#include "pico/stdlib.h"

/* 按鍵 -> 送出什麼鍵碼。
 *
 *   方向    游標移動;注音組字中則是移動候選(editor.c 自己分辨)
 *   A       Enter —— 選字、換行
 *   B       Esc   —— 取消組字
 *   START   Enter
 *   SELECT  PgDn  —— 候選字翻頁
 *
 * A/B 對應 Enter/Esc 是照載入器選單的習慣(A 確定、B 取消)。
 */
static const struct { uint gpio; uint8_t code; } BTN[] = {
    { BTN_PIN_UP,     KEY_UP    },
    { BTN_PIN_DOWN,   KEY_DOWN  },
    { BTN_PIN_LEFT,   KEY_LEFT  },
    { BTN_PIN_RIGHT,  KEY_RIGHT },
    { BTN_PIN_A,      KEY_ENTER },
    { BTN_PIN_B,      KEY_ESC   },
    { BTN_PIN_START,  KEY_ENTER },
    { BTN_PIN_SELECT, KEY_PGDN  },
};
#define BTN_N ((int)(sizeof BTN / sizeof BTN[0]))

static uint8_t  stable;                 /* 去彈跳後的狀態,每顆一個 bit */
static uint8_t  pending;                /* 上一次讀到的原始狀態 */
static uint32_t changed_ms[BTN_N];
static int      repeat_i = -1;          /* 正在連發的那顆 */
static uint32_t repeat_at;

void hw_dpad_init(void)
{
    for (int i = 0; i < BTN_N; i++) {
        gpio_init(BTN[i].gpio);
        gpio_set_dir(BTN[i].gpio, GPIO_IN);
        gpio_pull_up(BTN[i].gpio);      /* active-low */
    }
    stable = pending = 0;
    repeat_i = -1;
}

static int emit(int n, key_event *out, int max, uint8_t code, int repeat)
{
    if (n >= max) return n;             /* 佇列滿了就丟掉 —— 掃描頻率遠高於人手 */
    out[n].code = code;
    out[n].mods = 0;
    out[n].repeat = (uint8_t)repeat;
    return n + 1;
}

int hw_dpad_poll(int n, key_event *out, int max)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint8_t raw = 0;

    for (int i = 0; i < BTN_N; i++)
        if (!gpio_get(BTN[i].gpio))     /* active-low:拉低 = 按下 */
            raw |= (uint8_t)(1u << i);

    for (int i = 0; i < BTN_N; i++) {
        uint8_t bit = (uint8_t)(1u << i);
        int now_down = (raw & bit) != 0;
        int was_down = (pending & bit) != 0;
        int is_stable = (stable & bit) != 0;

        if (now_down != was_down) {
            changed_ms[i] = now;        /* 狀態剛變,開始計時 */
        } else if (now_down != is_stable &&
                   (int32_t)(now - changed_ms[i]) >= KEYS_DEBOUNCE_MS) {
            /* 穩定超過去彈跳窗才承認 */
            stable = (uint8_t)(now_down ? (stable | bit) : (stable & ~bit));
            if (now_down) {
                n = emit(n, out, max, BTN[i].code, 0);
                repeat_i = i;
                repeat_at = now + KEYS_REPEAT_MS;
            } else if (repeat_i == i) {
                repeat_i = -1;
            }
        } else if (now_down && is_stable && repeat_i == i &&
                   (int32_t)(now - repeat_at) >= 0) {
            /* 按住不放 -> 連發。方向鍵沒有連發的話,游標得一下一下點。 */
            n = emit(n, out, max, BTN[i].code, 1);
            repeat_at = now + KEYS_RATE_MS;
        }
    }

    pending = raw;
    return n;
}
