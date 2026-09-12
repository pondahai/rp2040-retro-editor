/* rp2040-retro-editor - 掌機上的注音文件編輯器
 *
 * 同時是 rp2040-retro-loader 的 app 範本。分層刻意切得很死:
 *
 *     core/    純 C,不碰硬體,PC 上可測(textbuf / editor / glyph)
 *     vendor/  從生態系其他專案搬來、已在真機驗證過的(lcd / keys / ime /
 *              font_cjk / fatfs / sdcard)
 *     src/     只有這一層碰 GPIO
 *
 * 這個分法是從 rp2040-retro-dict 學來的:真正容易寫錯的邏輯(UTF-8 邊界、
 * 去彈跳、注音組字、字型的位元順序)全部留在能寫測試的那一側。
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "board.h"
#include "hw_display.h"
#include "hw_keys.h"
#include "doc_sd.h"
#include "editor.h"

/* 文件緩衝。32KB 在 RP2040 的 264KB SRAM 裡算客氣,中文一個字 3 bytes,
 * 大約一萬字。gap buffer 不搬家的話插入是 O(1)。 */
static char doc_storage[32 * 1024];

static editor ed;
static keys   kb;

#define DOC_NAME "NOTE.TXT"

static const char WELCOME[] =
    "rp2040-retro-editor\n"
    "\n"
    "Fn+Space  ABC / BOPOMOFO\n"
    "Fn+S      save\n";

static void set_msg(const char *m)
{
    strncpy(ed.msg, m, sizeof ed.msg - 1);
    ed.msg[sizeof ed.msg - 1] = 0;
}

/* 存檔。
 *
 * gap buffer 平常是斷成兩截的,但把游標移到最尾端之後 gap 就整塊跑到後面去,
 * buf[0..len) 剛好是連續的文字 —— 不必另外開一塊 32KB 來拼接。
 * 存完再把游標移回原位。 */
static void save_doc(void)
{
    size_t cur = tb_cursor(&ed.tb);
    size_t len = tb_len(&ed.tb);
    int r;

    tb_move_to(&ed.tb, len);
    r = doc_sd_save(ed.name, ed.tb.buf, len);
    tb_move_to(&ed.tb, cur);

    if (r == DOC_OK) {
        ed.tb.dirty = 0;
        set_msg("saved");
    } else {
        set_msg(doc_sd_strerror(r));
    }
    ed_reflow(&ed);
}

int main(void)
{
    stdio_init_all();

    hw_display_init();
    hw_keys_init();
    keys_init(&kb);

    ed_init(&ed, doc_storage, sizeof doc_storage, DOC_NAME);

    if (doc_sd_init() != DOC_OK) {
        /* 沒卡也要能用 —— 至少讓使用者看得到畫面是活的,而不是一片黑 */
        tb_load(&ed.tb, WELCOME, strlen(WELCOME));
        set_msg("no SD card");
    } else {
        int n = doc_sd_load(DOC_NAME, doc_storage, sizeof doc_storage);
        if (n >= 0) {
            /* doc_sd_load 直接讀進 storage,所以這裡只要把 gap 補回來 */
            ed.tb.gap = (size_t)n;
            ed.tb.gap_end = ed.tb.cap;
            set_msg("loaded");
        } else if (n == DOC_ENOENT) {
            set_msg("new file");
        } else {
            tb_load(&ed.tb, WELCOME, strlen(WELCOME));
            set_msg(doc_sd_strerror(n));
        }
    }

    tb_move_to(&ed.tb, 0);
    ed.tb.dirty = 0;
    ed_reflow(&ed);
    hw_display_draw(&ed);

    for (;;) {
        key_event evs[KEYS_MAX_EVENTS];
        int n = hw_keys_poll(&kb, evs, KEYS_MAX_EVENTS);

        for (int i = 0; i < n; i++)
            ed_key(&ed, &evs[i]);

        if (ed.need_save) {
            ed.need_save = 0;
            save_doc();
            ed.redraw = 1;
        }

        if (ed.redraw) {
            ed.redraw = 0;
            hw_display_draw(&ed);
        }

        sleep_ms(5);    /* 掃描頻率 ~200Hz,遠高於 30ms 的去彈跳窗 */
    }
}
