/* rp2040-retro-editor - 掌機上的注音文件編輯器
 *
 * 同時是 rp2040-retro-loader 的 app 範本。分層刻意切得很死：
 *
 *     core/    純 C,不碰硬體,PC 上可測(textbuf / editor)
 *     vendor/  從生態系其他專案搬來、已在真機驗證過的(lcd / keys / ime)
 *     src/     只有這一層碰 GPIO
 *
 * 這個分法是從 rp2040-retro-dict 學來的:真正容易寫錯的邏輯(UTF-8 邊界、
 * 去彈跳、注音組字)全部留在能寫測試的那一側。
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

static const char WELCOME[] =
    "rp2040-retro-editor\n"
    "\n"
    "Fn+Space  switch ABC / BOPOMOFO\n"
    "Fn+S      save\n"
    "\n"
    "SD card is not wired up yet.\n";

int main(void)
{
    stdio_init_all();

    hw_display_init();
    hw_keys_init();
    keys_init(&kb);

    ed_init(&ed, doc_storage, sizeof doc_storage, "untitled.txt");

    /* SD 還沒接上(見 doc_sd.h)。先放一段開機訊息,讓畫面上有東西可看、
     * 也順便驗證 LCD 與鍵盤這兩層是活的。 */
    if (doc_sd_init() != DOC_OK) {
        tb_load(&ed.tb, WELCOME, strlen(WELCOME));
        tb_move_to(&ed.tb, 0);
        ed.tb.dirty = 0;
        ed_reflow(&ed);
    }

    hw_display_draw(&ed);

    for (;;) {
        key_event evs[KEYS_MAX_EVENTS];
        int n = hw_keys_poll(&kb, evs, KEYS_MAX_EVENTS);

        for (int i = 0; i < n; i++)
            ed_key(&ed, &evs[i]);

        if (ed.need_save) {
            ed.need_save = 0;
            int r = doc_sd_save(ed.name, NULL, 0);   /* 還沒接上 */
            if (r == DOC_ENOIMPL)
                strncpy(ed.msg, "SD not wired up", sizeof ed.msg - 1);
            ed.redraw = 1;
        }

        if (ed.redraw) {
            ed.redraw = 0;
            hw_display_draw(&ed);
        }

        sleep_ms(5);    /* 掃描頻率 ~200Hz,遠高於 30ms 的去彈跳窗 */
    }
}
