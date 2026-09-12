/* editor - 編輯器的狀態機。純 C，不碰硬體。
 *
 * 輸入是 vendor/keys.c 產生的 key_event，輸出是「畫面該長什麼樣」。
 * 繪製與 SD 存取都在外面，所以整個狀態機可以在 PC 上測。
 *
 * 注音組字的狀態也在這裡：使用者打的按鍵串 -> vendor/ime.c 查候選 ->
 * 選一個字 -> 插進 textbuf。
 */
#ifndef EDITOR_H
#define EDITOR_H

#include "textbuf.h"
#include "keys.h"
#include "ime.h"

#define ED_COLS 40           /* 跟 lcd.h 的文字模式一致 */
#define ED_ROWS 30
#define ED_TEXT_ROWS (ED_ROWS - 2)   /* 扣掉頂端狀態列與底部候選列 */

#define ED_CAND_MAX 9        /* 一次顯示幾個候選字（用數字鍵 1-9 選） */
#define ED_NAME_MAX 32

typedef enum {
    ED_MODE_ASCII = 0,       /* 英數直接輸入 */
    ED_MODE_BOPO             /* 注音組字中 */
} ed_mode;

typedef struct {
    textbuf  tb;
    ed_mode  mode;

    /* --- 注音組字狀態 --- */
    char     comp[IME_MAX_KEYS + 1];  /* 使用者打的按鍵串，例如 "su3" */
    int      comp_len;
    char     cands[256];              /* ime_query 的結果（多個字接在一起） */
    int      cand_count;
    int      cand_page;               /* 第幾頁，一頁 ED_CAND_MAX 個 */

    /* --- 畫面 --- */
    size_t   top;                     /* 畫面最上面那一行的 byte offset */
    int      cur_row, cur_col;        /* 游標在畫面上的格子（繪製用） */

    /* --- 檔案 --- */
    char     name[ED_NAME_MAX];
    uint8_t  need_save;               /* 主迴圈看到這個就去寫 SD */
    uint8_t  redraw;                  /* 內容變了，要重畫 */

    char     msg[ED_COLS + 1];        /* 狀態列訊息，空字串 = 沒事 */
} editor;

void ed_init(editor *ed, char *storage, size_t cap, const char *name);

/* 餵一個按鍵事件。回傳 1 = 狀態有變（該重畫）。 */
int  ed_key(editor *ed, const key_event *ev);

/* 重新算 top 與 cur_row/cur_col，讓游標留在畫面內。ed_key 內部會呼叫，
 * 載入檔案之後也要自己叫一次。 */
void ed_reflow(editor *ed);

/* 目前這一頁有幾個候選字可選（0 = 沒在組字）。 */
int  ed_cand_count(const editor *ed);
/* 取這一頁第 n 個候選字（UTF-8，NUL 結尾）。回傳 byte 數，0 = 沒有。 */
int  ed_cand_nth(const editor *ed, int n, char *out, int out_size);

#endif /* EDITOR_H */
