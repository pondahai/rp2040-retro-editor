#include "hw_display.h"
#include "lcd.h"
#include <string.h>
#include <stdio.h>

#define ROW_STATUS 0
#define ROW_TEXT   1
#define ROW_CAND   (LCD_ROWS - 1)

void hw_display_init(void)
{
    lcd_init();
    lcd_clear(C_BLACK);
}

/* 中文字的佔位方塊。字型接上之前，至少讓使用者看得到「這裡有一個字」，
 * 而且寬度要跟真的中文字一樣是兩格 —— 不然游標位置會對不上。 */
static void put_cjk_box(int col, int row, uint16_t fg)
{
    lcd_fill_rect(col * 8 + 1, row * 8 + 1, 14, 6, fg);
}

/* 畫一行 UTF-8。回傳畫了幾格。 */
static int draw_utf8(int col, int row, const char *s, size_t len,
                     uint16_t fg, uint16_t bg)
{
    size_t i = 0;
    while (i < len && col < LCD_COLS) {
        unsigned char c = (unsigned char)s[i];
        int seq = utf8_seq_len(c);
        if (seq == 1) {
            lcd_putc(col, row, (char)c, fg, bg);
            col += 1;
        } else {
            put_cjk_box(col, row, fg);
            col += 2;
        }
        i += (size_t)seq;
    }
    return col;
}

static void draw_status(const editor *ed)
{
    /* 比一行寬 —— 檔名與訊息合起來可能超過 40 格，靠 lcd_puts_line 截掉。
     * 開成剛好 LCD_COLS+1 的話 snprintf 會先截一次，gcc 也會抱怨。 */
    char line[ED_NAME_MAX + LCD_COLS + 16];
    snprintf(line, sizeof line, "%-16s %s%s %s",
             ed->name,
             ed->mode == ED_MODE_BOPO ? "BOPO" : "ABC ",
             ed->tb.dirty ? "*" : " ",
             ed->msg);
    lcd_puts_line(ROW_STATUS, line, C_BLACK, C_GREY);
}

static void draw_text(const editor *ed)
{
    char buf[LCD_COLS * 3 + 1];
    size_t p = ed->top;
    size_t total = tb_len(&ed->tb);

    for (int row = 0; row < ED_TEXT_ROWS; row++) {
        int screen_row = ROW_TEXT + row;
        lcd_puts_line(screen_row, "", C_WHITE, C_BLACK);   /* 先清掉殘字 */

        if (p > total) continue;

        size_t end = tb_line_end(&ed->tb, p);
        size_t n = end - p;
        if (n > sizeof buf - 1) n = sizeof buf - 1;
        n = tb_copy(&ed->tb, p, buf, n);

        draw_utf8(0, screen_row, buf, n, C_WHITE, C_BLACK);

        if (end >= total) { p = total + 1; }   /* 最後一行畫完就停 */
        else p = end + 1;
    }
}

static void draw_cursor(const editor *ed)
{
    int col = ed->cur_col;
    int row = ROW_TEXT + ed->cur_row;
    if (col >= LCD_COLS) col = LCD_COLS - 1;
    if (row >= ROW_CAND) return;
    lcd_fill_rect(col * 8, row * 8 + 7, 8, 1, C_YELLOW);
}

static void draw_cands(const editor *ed)
{
    char line[LCD_COLS + 1];
    int n = ed_cand_count(ed);
    int col;

    lcd_puts_line(ROW_CAND, "", C_BLACK, C_BLUE);
    if (ed->mode != ED_MODE_BOPO) return;

    /* 左邊先寫使用者打的注音符號 */
    {
        char bopo[IME_MAX_BOPO + 1];
        int len = ime_bopomofo(ed->comp, bopo, sizeof bopo);
        bopo[len] = 0;
        col = draw_utf8(0, ROW_CAND, bopo, (size_t)len, C_YELLOW, C_BLUE);
        col += 1;
    }

    /* 右邊列候選字，前面標 1-9 */
    for (int i = 0; i < n && col < LCD_COLS - 3; i++) {
        char ch[8];
        int len = ed_cand_nth(ed, i, ch, sizeof ch);
        snprintf(line, sizeof line, "%d", i + 1);
        lcd_puts(col, ROW_CAND, line, C_GREY, C_BLUE);
        col += 1;
        col = draw_utf8(col, ROW_CAND, ch, (size_t)len, C_WHITE, C_BLUE);
        col += 1;
    }
}

void hw_display_draw(const editor *ed)
{
    draw_status(ed);
    draw_text(ed);
    draw_cursor(ed);
    draw_cands(ed);
}
