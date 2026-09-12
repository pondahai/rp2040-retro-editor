/* core/textbuf.c 的 PC 端測試。不需要板子、不需要 SDK。
 *
 *   cc -I../core -o test_textbuf test_textbuf.c ../core/textbuf.c && ./test_textbuf
 *
 * 會錯的地方都在多位元組字元的邊界上，所以測試用中文而不是 "abc"。
 */
#include "textbuf.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); fails++; } \
} while (0)

static void dump(const textbuf *tb, char *out, size_t n)
{
    size_t got = tb_copy(tb, 0, out, n - 1);
    out[got] = 0;
}

#define CHECK_TEXT(tb, want) do { \
    char _b[256]; dump(tb, _b, sizeof _b); \
    if (strcmp(_b, want) != 0) { \
        printf("  FAIL 內容 = \"%s\", 應為 \"%s\" (%s:%d)\n", _b, want, __FILE__, __LINE__); \
        fails++; \
    } \
} while (0)

static void t_insert_ascii(void)
{
    char store[64]; textbuf tb;
    printf("insert ascii\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "abc", 3);
    CHECK_TEXT(&tb, "abc");
    CHECK(tb_len(&tb) == 3, "len == 3");
    CHECK(tb_cursor(&tb) == 3, "游標在尾端");
}

static void t_insert_cjk(void)
{
    char store[64]; textbuf tb;
    printf("insert 中文\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "你", 3);
    tb_insert(&tb, "好", 3);
    CHECK_TEXT(&tb, "你好");
    CHECK(tb_len(&tb) == 6, "兩個中文字 = 6 bytes");
}

static void t_backspace_is_one_char(void)
{
    char store[64]; textbuf tb;
    printf("backspace 刪一個字而不是一個 byte\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "你好", 6);
    CHECK(tb_backspace(&tb) == 3, "刪掉 3 bytes");
    CHECK_TEXT(&tb, "你");
    CHECK(tb_backspace(&tb) == 3, "再刪 3 bytes");
    CHECK_TEXT(&tb, "");
    CHECK(tb_backspace(&tb) == 0, "空的時候刪不動");
}

static void t_cursor_never_splits_char(void)
{
    char store[64]; textbuf tb;
    printf("游標不會停在中文字中間\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "你好嗎", 9);
    tb_left(&tb);
    CHECK(tb_cursor(&tb) == 6, "左移一次到 6（不是 8）");
    tb_left(&tb);
    CHECK(tb_cursor(&tb) == 3, "再左移到 3");
    tb_right(&tb);
    CHECK(tb_cursor(&tb) == 6, "右移回 6");

    /* 直接把游標丟進字元中間，應該被拉回邊界 */
    tb_move_to(&tb, 7);
    CHECK(tb_cursor(&tb) == 6, "move_to(7) 對齊回 6");
}

static void t_insert_in_middle(void)
{
    char store[64]; textbuf tb;
    printf("在中間插入（gap 要搬對）\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "你嗎", 6);
    tb_move_to(&tb, 3);
    tb_insert(&tb, "好", 3);
    CHECK_TEXT(&tb, "你好嗎");
}

static void t_delete_forward(void)
{
    char store[64]; textbuf tb;
    printf("delete 往右刪一個字\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "你好", 6);
    tb_move_to(&tb, 0);
    CHECK(tb_delete(&tb) == 3, "刪掉 3 bytes");
    CHECK_TEXT(&tb, "好");
}

static void t_lines(void)
{
    char store[64]; textbuf tb;
    printf("行的邊界\n");
    tb_init(&tb, store, sizeof store);
    tb_insert(&tb, "第一行\n第二行", 19);
    CHECK(tb_line_count(&tb) == 2, "兩行");
    tb_move_to(&tb, 12);                 /* 第二行中間 */
    CHECK(tb_line_start(&tb, 12) == 10, "行首在 10");
    tb_home(&tb);
    CHECK(tb_cursor(&tb) == 10, "home 到行首");
    tb_end(&tb);
    CHECK(tb_cursor(&tb) == 19, "end 到行尾");
}

static void t_full_buffer(void)
{
    char store[8]; textbuf tb;
    printf("緩衝區滿了要擋住，不是寫爆\n");
    tb_init(&tb, store, sizeof store);
    CHECK(tb_insert(&tb, "12345678", 8) == 0, "剛好塞滿");
    CHECK(tb_insert(&tb, "9", 1) == -1, "再塞就拒絕");
    CHECK(tb_len(&tb) == 8, "長度沒被寫壞");
}

static void t_load_roundtrip(void)
{
    char store[64]; textbuf tb;
    printf("load 之後編輯\n");
    tb_init(&tb, store, sizeof store);
    CHECK(tb_load(&tb, "你好", 6) == 0, "載入成功");
    tb_move_to(&tb, 0);
    CHECK(tb_cursor(&tb) == 0, "游標移到開頭");
    tb_insert(&tb, "嗨", 3);
    CHECK_TEXT(&tb, "嗨你好");
    CHECK(tb.dirty == 1, "改過了要標記 dirty");
}

static void t_utf8_helpers(void)
{
    printf("utf8 小工具\n");
    CHECK(utf8_seq_len('a') == 1, "ASCII 長度 1");
    CHECK(utf8_seq_len((unsigned char)"\xE4"[0]) == 3, "中文首位元組長度 3");
    CHECK(utf8_is_cont((unsigned char)"\xBD"[0]) == 1, "接續位元組");
    CHECK(utf8_is_cont('a') == 0, "ASCII 不是接續位元組");
    CHECK(utf8_seq_len(0xFF) == 1, "非法首位元組不要卡住");
}

int main(void)
{
    t_utf8_helpers();
    t_insert_ascii();
    t_insert_cjk();
    t_backspace_is_one_char();
    t_cursor_never_splits_char();
    t_insert_in_middle();
    t_delete_forward();
    t_lines();
    t_full_buffer();
    t_load_roundtrip();

    if (fails == 0) { printf("\n全部通過\n"); return 0; }
    printf("\n%d 項失敗\n", fails);
    return 1;
}
