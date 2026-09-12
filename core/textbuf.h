/* textbuf - UTF-8 文字緩衝（gap buffer）。
 *
 * 純 C，不碰硬體、不 malloc：緩衝區由呼叫端給。跟 vendor/keys.c 與
 * vendor/ime.c 一樣的原則 —— 真正容易寫錯的邏輯（游標在多位元組字
 * 中間、行首行尾、gap 移動）全部能在 PC 上測。
 *
 * 座標一律用 byte offset。對外保證游標永遠落在 UTF-8 字元邊界上。
 */
#ifndef TEXTBUF_H
#define TEXTBUF_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char    *buf;        /* 呼叫端提供 */
    size_t   cap;        /* buf 的總 bytes */
    size_t   gap;        /* gap 起點 = 游標位置（邏輯 offset） */
    size_t   gap_end;    /* gap 終點（不含）；文字量 = cap - (gap_end - gap) */
    uint8_t  dirty;      /* 自上次存檔後有沒有改過 */
} textbuf;

/* ---- 建立 ---- */
void   tb_init(textbuf *tb, char *storage, size_t cap);
/* 把既有內容載入（覆蓋）。回傳 0 = 成功，-1 = 放不下。 */
int    tb_load(textbuf *tb, const char *data, size_t len);

/* ---- 查詢 ---- */
size_t tb_len(const textbuf *tb);           /* 文字總 bytes */
size_t tb_cursor(const textbuf *tb);        /* 游標的邏輯 offset */
/* 邏輯 offset -> 該位置的 byte。超出範圍回 0。 */
char   tb_at(const textbuf *tb, size_t pos);
/* 把 [pos, pos+n) 複製到 out。回傳實際複製的 bytes。存檔與繪製都走這裡。 */
size_t tb_copy(const textbuf *tb, size_t pos, char *out, size_t n);

/* ---- 編輯 ---- */
/* 插入一段 UTF-8（一個注音選出來的字、或一個 ASCII 字元）。
 * 回傳 0 = 成功，-1 = 空間不足。 */
int    tb_insert(textbuf *tb, const char *utf8, size_t len);
/* 刪除游標左邊一個「字元」（不是一個 byte）。回傳刪掉的 bytes。 */
int    tb_backspace(textbuf *tb);
/* 刪除游標右邊一個字元。回傳刪掉的 bytes。 */
int    tb_delete(textbuf *tb);

/* ---- 游標 ---- */
void   tb_move_to(textbuf *tb, size_t pos);   /* 會自動對齊到字元邊界 */
void   tb_left(textbuf *tb);                  /* 一個字元，不是一個 byte */
void   tb_right(textbuf *tb);
void   tb_home(textbuf *tb);                  /* 行首 */
void   tb_end(textbuf *tb);                   /* 行尾 */

/* ---- 行 ---- */
size_t tb_line_start(const textbuf *tb, size_t pos);  /* pos 所在行的行首 */
size_t tb_line_end(const textbuf *tb, size_t pos);    /* 行尾（指向 '\n' 或 EOF） */
size_t tb_line_count(const textbuf *tb);

/* ---- UTF-8 小工具（獨立可測） ---- */
int    utf8_seq_len(unsigned char first);   /* 由首位元組判長度，非法回 1 */
int    utf8_is_cont(unsigned char b);       /* 是不是接續位元組 10xxxxxx */

#endif /* TEXTBUF_H */
