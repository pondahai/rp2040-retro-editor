#include "textbuf.h"
#include <string.h>

/* gap buffer 的排列：
 *
 *     [ 0 .. gap )      游標之前的文字
 *     [ gap .. gap_end) 空隙（垃圾）
 *     [ gap_end .. cap) 游標之後的文字
 *
 * 邏輯 offset p 對應的實體位置：p < gap ? p : p + (gap_end - gap)
 */

#define GAPSZ(tb) ((tb)->gap_end - (tb)->gap)

static size_t phys(const textbuf *tb, size_t p)
{
    return p < tb->gap ? p : p + GAPSZ(tb);
}

int utf8_is_cont(unsigned char b) { return (b & 0xC0) == 0x80; }

int utf8_seq_len(unsigned char f)
{
    if (f < 0x80) return 1;
    if ((f & 0xE0) == 0xC0) return 2;
    if ((f & 0xF0) == 0xE0) return 3;   /* 中文都走這條 */
    if ((f & 0xF8) == 0xF0) return 4;
    return 1;                            /* 非法首位元組：當一個 byte 吃掉，不要卡住 */
}

void tb_init(textbuf *tb, char *storage, size_t cap)
{
    tb->buf = storage;
    tb->cap = cap;
    tb->gap = 0;
    tb->gap_end = cap;
    tb->dirty = 0;
}

size_t tb_len(const textbuf *tb) { return tb->cap - GAPSZ(tb); }
size_t tb_cursor(const textbuf *tb) { return tb->gap; }

char tb_at(const textbuf *tb, size_t pos)
{
    if (pos >= tb_len(tb)) return 0;
    return tb->buf[phys(tb, pos)];
}

size_t tb_copy(const textbuf *tb, size_t pos, char *out, size_t n)
{
    size_t len = tb_len(tb), got = 0;
    if (pos >= len) return 0;
    if (n > len - pos) n = len - pos;

    /* 前段：[pos, gap) */
    if (pos < tb->gap) {
        size_t chunk = tb->gap - pos;
        if (chunk > n) chunk = n;
        memcpy(out, tb->buf + pos, chunk);
        got += chunk;
        pos += chunk;
    }
    /* 後段：跨過 gap */
    if (got < n) {
        size_t chunk = n - got;
        memcpy(out + got, tb->buf + phys(tb, pos), chunk);
        got += chunk;
    }
    return got;
}

int tb_load(textbuf *tb, const char *data, size_t len)
{
    if (len > tb->cap) return -1;
    memcpy(tb->buf, data, len);
    tb->gap = len;              /* 游標放在最後，稍後由呼叫端移到開頭 */
    tb->gap_end = tb->cap;
    tb->dirty = 0;
    return 0;
}

/* 把 gap 搬到 pos。這是 gap buffer 唯一會搬記憶體的地方。 */
void tb_move_to(textbuf *tb, size_t pos)
{
    size_t len = tb_len(tb);
    if (pos > len) pos = len;

    /* 對齊到字元邊界：往左退到非接續位元組 */
    while (pos > 0 && utf8_is_cont((unsigned char)tb_at(tb, pos))) pos--;

    if (pos < tb->gap) {
        /* gap 往左：把 [pos, gap) 的文字搬到 gap 尾端 */
        size_t n = tb->gap - pos;
        memmove(tb->buf + tb->gap_end - n, tb->buf + pos, n);
        tb->gap -= n;
        tb->gap_end -= n;
    } else if (pos > tb->gap) {
        /* gap 往右：把 gap 後面 n 個 byte 搬到 gap 前面 */
        size_t n = pos - tb->gap;
        memmove(tb->buf + tb->gap, tb->buf + tb->gap_end, n);
        tb->gap += n;
        tb->gap_end += n;
    }
}

int tb_insert(textbuf *tb, const char *utf8, size_t len)
{
    if (GAPSZ(tb) < len) return -1;
    memcpy(tb->buf + tb->gap, utf8, len);
    tb->gap += len;
    tb->dirty = 1;
    return 0;
}

int tb_backspace(textbuf *tb)
{
    size_t n = 0;
    if (tb->gap == 0) return 0;
    /* 往左吃掉接續位元組，再吃掉首位元組 —— 一次刪一個字元 */
    do {
        tb->gap--;
        n++;
    } while (tb->gap > 0 && n < 4 && utf8_is_cont((unsigned char)tb->buf[tb->gap]));
    tb->dirty = 1;
    return (int)n;
}

int tb_delete(textbuf *tb)
{
    int n;
    if (tb->gap_end >= tb->cap) return 0;
    n = utf8_seq_len((unsigned char)tb->buf[tb->gap_end]);
    if ((size_t)n > tb->cap - tb->gap_end) n = (int)(tb->cap - tb->gap_end);
    tb->gap_end += n;
    tb->dirty = 1;
    return n;
}

void tb_left(textbuf *tb)
{
    if (tb->gap == 0) return;
    tb_move_to(tb, tb->gap - 1);   /* tb_move_to 自己會對齊邊界 */
}

void tb_right(textbuf *tb)
{
    size_t len = tb_len(tb);
    int n;
    if (tb->gap >= len) return;
    n = utf8_seq_len((unsigned char)tb_at(tb, tb->gap));
    tb_move_to(tb, tb->gap + n);
}

size_t tb_line_start(const textbuf *tb, size_t pos)
{
    while (pos > 0 && tb_at(tb, pos - 1) != '\n') pos--;
    return pos;
}

size_t tb_line_end(const textbuf *tb, size_t pos)
{
    size_t len = tb_len(tb);
    while (pos < len && tb_at(tb, pos) != '\n') pos++;
    return pos;
}

void tb_home(textbuf *tb) { tb_move_to(tb, tb_line_start(tb, tb->gap)); }
void tb_end(textbuf *tb)  { tb_move_to(tb, tb_line_end(tb, tb->gap)); }

size_t tb_line_count(const textbuf *tb)
{
    size_t i, len = tb_len(tb), n = 1;
    for (i = 0; i < len; i++) if (tb_at(tb, i) == '\n') n++;
    return n;
}
