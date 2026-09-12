#include "hw_display.h"
#include "glyph.h"
#include "lcd.h"
#include <string.h>
#include <stdio.h>

/* 版面(格子是 8 寬 x 16 高,見 core/glyph.h):
 *
 *     row 0        狀態列
 *     row 1..13    內文
 *     row 14       注音組字與候選字
 *
 * 不用 lcd.c 的 8x8 文字模式 —— 中文字是 16 高,跟那個格線對不上。
 * 改走它的像素模式(lcd_blit),一次送一格 8x16。仍然沒有 framebuffer:
 * 320x240x16bpp 是 150KB,而且 DMA 留在半路會弄髒交棒後的下一個專題。
 */
/* 候選列的版面(單位是格,一格 8px 寬):
 *
 *     0 .. CAND_COL-1    注音符號區
 *     CAND_COL ..        候選字,每個佔 CAND_CELLS 格 = 數字 1 + 中文 2
 *
 * 注音區給**固定**寬度而不是「畫多少算多少」,這樣候選字的位置不會隨著
 * 注音打到第幾個而左右跳動。
 *
 * 總寬 9 + 9x3 = 36 <= 40,剩 4 格餘裕。之前每個候選後面還多加一個空格
 * (4 格),9 + 9x4 = 43 就超出畫面 —— 真機上看到的「第九個候選跑出去」。
 */
#define CAND_COL    9
#define CAND_CELLS  3

#define ROW_STATUS 0
#define ROW_TEXT   1
#define ROW_CAND   (ED_ROWS - 1)

#define CELL_PX    (GLYPH_W * GLYPH_H)      /* 8 x 16 = 128 像素 */

void hw_display_init(void)
{
    lcd_init();
    lcd_clear(C_BLACK);
}

/* 畫一格。RGB565 big-endian —— 跟 ILI9341 的線上格式一致。 */
static void put_cell(int col, int row, glyph_ref g, int half,
                     uint16_t fg, uint16_t bg)
{
    uint8_t px[CELL_PX * 2];
    int i = 0;

    if (half) {
        /* 寬字的右半:同一個 glyph,換一種取法 */
        if (g.kind == GL_WIDE_LEAD) g.kind = GL_WIDE_TRAIL;
        else if (g.kind == GL_TOFU_LEAD) g.kind = GL_TOFU_TRAIL;
    }

    for (int y = 0; y < GLYPH_H; y++) {
        uint8_t slice = glyph_slice(g, y);
        for (int x = 0; x < GLYPH_W; x++) {
            uint16_t c = (slice & 1) ? fg : bg;
            slice >>= 1;                     /* bit 0 是最左邊 */
            px[i++] = (uint8_t)(c >> 8);
            px[i++] = (uint8_t)(c & 0xff);
        }
    }

    lcd_blit_begin(col * GLYPH_W, row * GLYPH_H, GLYPH_W, GLYPH_H);
    lcd_blit(px, sizeof px);
}

/* 畫一段 UTF-8,回傳畫到第幾格。 */
static int draw_utf8(int col, int row, const char *s, size_t len,
                     uint16_t fg, uint16_t bg)
{
    size_t i = 0;
    while (i < len && col < ED_COLS) {
        uint32_t cp;
        int n = utf8_decode(s + i, (int)(len - i), &cp);
        glyph_ref g;

        if (n <= 0) break;
        i += (size_t)n;
        if (cp == '\n') break;

        g = glyph_find(cp);
        put_cell(col++, row, g, 0, fg, bg);
        if (glyph_cells(cp) == 2 && col < ED_COLS)
            put_cell(col++, row, g, 1, fg, bg);
    }
    return col;
}

/* 把整列填成底色。
 *
 * 每一列都是**先清整列、再畫字**,不是畫完才去補清尾巴。
 *
 * 補清尾巴的寫法要靠「這次畫到第幾格」去算起點,那個數字一旦算錯
 * (中文佔兩格、行尾只剩一格、字串被截斷…)殘字就留在畫面上 —— 真機上
 * 回報過狀態列新舊訊息疊在一起。整列先清就沒有這個算式了。
 *
 * 成本是每列多一次 fill_rect,但這幾列只在 redraw 時畫,不是每幀。 */
static void clear_row(int row, uint16_t bg)
{
    lcd_fill_rect(0, row * GLYPH_H, ED_COLS * GLYPH_W, GLYPH_H, bg);
}

static void draw_status(const editor *ed)
{
    char line[ED_NAME_MAX + ED_COLS + 16];
    snprintf(line, sizeof line, "%s %s%s %s",
             ed->name,
             ed->mode == ED_MODE_BOPO ? "\xE6\xB3\xA8" : "A",   /* 注 / A */
             ed->tb.dirty ? "*" : " ",
             ed->msg);
    clear_row(ROW_STATUS, C_GREY);
    draw_utf8(0, ROW_STATUS, line, strlen(line), C_BLACK, C_GREY);
}

static void draw_text(const editor *ed)
{
    char buf[ED_COLS * 3 + 1];
    size_t p = ed->top;
    size_t total = tb_len(&ed->tb);

    for (int row = 0; row < ED_TEXT_ROWS; row++) {
        int screen_row = ROW_TEXT + row;

        clear_row(screen_row, C_BLACK);

        if (p <= total) {
            size_t end = tb_line_end(&ed->tb, p);
            size_t n = end - p;
            if (n > sizeof buf - 1) n = sizeof buf - 1;
            n = tb_copy(&ed->tb, p, buf, n);
            draw_utf8(0, screen_row, buf, n, C_WHITE, C_BLACK);
            p = (end >= total) ? total + 1 : end + 1;
        }
    }
}

static void draw_cursor(const editor *ed)
{
    int col = ed->cur_col;
    int row = ROW_TEXT + ed->cur_row;
    if (col >= ED_COLS) col = ED_COLS - 1;
    if (row >= ROW_CAND) return;
    lcd_fill_rect(col * GLYPH_W, row * GLYPH_H + GLYPH_H - 2,
                  GLYPH_W, 2, C_YELLOW);
}

static void draw_cands(const editor *ed)
{
    int n = ed_cand_count(ed);
    int col = 0;

    if (ed->mode != ED_MODE_BOPO) { clear_row(ROW_CAND, C_BLACK); return; }

    clear_row(ROW_CAND, C_BLUE);

    /* 左邊是使用者打的注音符號。碼表裡有 ㄅㄆㄇ 本身(U+3105..U+3129),
     * 所以這裡畫的是真的注音,不是佔位方塊。 */
    {
        char bopo[IME_MAX_BOPO + 1];
        int len = ime_bopomofo(ed->comp, bopo, sizeof bopo);
        bopo[len] = 0;
        draw_utf8(0, ROW_CAND, bopo, (size_t)len, C_YELLOW, C_BLUE);
    }
    col = CAND_COL;

    /* 右邊列候選字,標上 1-9。
     *
     * 數字是給 **Fn+數字** 用的,不是直接按數字 —— 直接按的話是注音鍵
     * (大千配列 1=ㄅ 2=ㄉ …)。keys.c 會把 Fn+數字轉成 F1~F9。
     * 目前反白的那個用 Enter 或 Space 確認。 */
    for (int i = 0; i < n; i++) {
        char ch[8], num[2];
        int len;
        int sel = (i == ed_cand_sel(ed));
        uint16_t fg = sel ? C_BLUE  : C_WHITE;
        uint16_t bg = sel ? C_WHITE : C_BLUE;

        /* 放不下就整個不畫 —— 畫一半比少一個更難看,而且中文字只畫左半
         * 會變成殘缺的字。 */
        if (col + CAND_CELLS > ED_COLS)
            break;

        len = ed_cand_nth(ed, i, ch, sizeof ch);
        num[0] = (char)('1' + i);
        num[1] = 0;
        draw_utf8(col, ROW_CAND, num, 1, C_GREY, C_BLUE);
        draw_utf8(col + 1, ROW_CAND, ch, (size_t)len, fg, bg);
        col += CAND_CELLS;
    }
}

void hw_display_draw(const editor *ed)
{
    draw_status(ed);
    draw_text(ed);
    draw_cursor(ed);
    draw_cands(ed);
}
