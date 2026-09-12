#include "editor.h"
#include <string.h>

static void set_msg(editor *ed, const char *m)
{
    strncpy(ed->msg, m, sizeof ed->msg - 1);
    ed->msg[sizeof ed->msg - 1] = 0;
}

void ed_init(editor *ed, char *storage, size_t cap, const char *name)
{
    memset(ed, 0, sizeof *ed);
    tb_init(&ed->tb, storage, cap);
    ed->mode = ED_MODE_ASCII;
    strncpy(ed->name, name, sizeof ed->name - 1);
    ed->redraw = 1;
}

/* ---------- 注音組字 ---------- */

static void comp_reset(editor *ed)
{
    ed->comp_len = 0;
    ed->comp[0] = 0;
    ed->cand_count = 0;
    ed->cand_page = 0;
    ed->cand_sel = 0;
    ed->cands[0] = 0;
}

/* 打完一個鍵就重查一次候選。組字串是空的就當作沒在組字。 */
static void comp_refresh(editor *ed)
{
    int n;
    if (ed->comp_len == 0) { comp_reset(ed); return; }

    n = ime_query(ed->comp, ed->cands, (int)sizeof ed->cands);
    ed->cands[n] = 0;
    ed->cand_page = 0;
    ed->cand_sel = 0;

    /* 候選是一串接在一起的 UTF-8，要自己數有幾個字 */
    ed->cand_count = 0;
    {
        char tmp[8];
        while (ime_nth(ed->cands, ed->cand_count, tmp, sizeof tmp) > 0)
            ed->cand_count++;
    }
}

int ed_cand_count(const editor *ed)
{
    int rest;
    if (ed->mode != ED_MODE_BOPO || ed->cand_count == 0) return 0;
    rest = ed->cand_count - ed->cand_page * ED_CAND_MAX;
    if (rest < 0) rest = 0;
    return rest > ED_CAND_MAX ? ED_CAND_MAX : rest;
}

int ed_cand_nth(const editor *ed, int n, char *out, int out_size)
{
    if (n < 0 || n >= ed_cand_count(ed)) return 0;
    return ime_nth(ed->cands, ed->cand_page * ED_CAND_MAX + n, out, out_size);
}

int ed_cand_sel(const editor *ed)
{
    return ed->cand_sel;
}

static void cand_set_page(editor *ed, int page)
{
    int pages;
    if (ed->cand_count == 0) return;
    pages = (ed->cand_count + ED_CAND_MAX - 1) / ED_CAND_MAX;
    if (page < 0) page = pages - 1;
    if (page >= pages) page = 0;
    ed->cand_page = page;
    ed->cand_sel = 0;
}

/* 選中這一頁第 n 個候選字，插進文件。 */
static int commit_cand(editor *ed, int n)
{
    char ch[8];
    int len = ed_cand_nth(ed, n, ch, sizeof ch);
    if (len <= 0) return 0;
    if (tb_insert(&ed->tb, ch, (size_t)len) != 0) {
        set_msg(ed, "緩衝區滿了");
        return 0;
    }
    comp_reset(ed);
    return 1;
}

/* ---------- 畫面捲動 ---------- */

void ed_reflow(editor *ed)
{
    size_t cur = tb_cursor(&ed->tb);
    size_t p;
    int row;

    /* 游標在 top 之前 -> 往上捲到游標那一行 */
    if (cur < ed->top) ed->top = tb_line_start(&ed->tb, cur);

    /* 從 top 往下數，看游標落在第幾列 */
    for (;;) {
        p = ed->top;
        row = 0;
        while (row < ED_TEXT_ROWS) {
            size_t end = tb_line_end(&ed->tb, p);
            if (cur >= p && cur <= end) break;      /* 找到了 */
            if (end >= tb_len(&ed->tb)) break;
            p = end + 1;
            row++;
        }
        if (row < ED_TEXT_ROWS) break;
        /* 游標在畫面下方 -> top 往下推一行，再試 */
        ed->top = tb_line_end(&ed->tb, ed->top);
        if (ed->top < tb_len(&ed->tb)) ed->top++;
    }

    ed->cur_row = row;

    /* 行內第幾格：一個中文字算兩格，跟 8x8 等寬字型的排法一致 */
    {
        size_t i = tb_line_start(&ed->tb, cur);
        int col = 0;
        while (i < cur) {
            unsigned char c = (unsigned char)tb_at(&ed->tb, i);
            int seq = utf8_seq_len(c);
            col += (seq > 1) ? 2 : 1;
            i += (size_t)seq;
        }
        ed->cur_col = col;
    }
}

/* ---------- 按鍵處理 ---------- */

static int handle_bopo(editor *ed, const key_event *ev)
{
    uint8_t c = ev->code;
    int n = ed_cand_count(ed);

    /* ⚠ 這裡刻意**不用數字鍵選字**。
     *
     * 大千配列的數字鍵本身就是注音:1=ㄅ 2=ㄉ 5=ㄓ 8=ㄚ 9=ㄞ 0=ㄢ,
     * 3/4/6/7 是聲調。拿來選字的話數字鍵永遠打不出注音 —— 這是本專案
     * 真機上第一個被回報的 bug。
     *
     * 選字的操作照 rp2040-retro-dict 的 firmware/app.c:495-530,
     * 那一套已經在真機上用過:Enter 選字、上下移動、PgUp/PgDn 翻頁。
     */

    /* Enter:選目前這個候選 */
    if (c == KEY_ENTER && n > 0) {
        commit_cand(ed, ed->cand_sel);
        return 1;
    }

    /* 上下移動候選 */
    if (c == KEY_UP && n > 0) {
        if (ed->cand_sel > 0) ed->cand_sel--;
        return 1;
    }
    if (c == KEY_DOWN && n > 0) {
        if (ed->cand_sel + 1 < n) ed->cand_sel++;
        return 1;
    }

    /* 翻頁 */
    if (c == KEY_PGDN && ed->cand_count > 0) {
        cand_set_page(ed, ed->cand_page + 1);
        return 1;
    }
    if (c == KEY_PGUP && ed->cand_count > 0) {
        cand_set_page(ed, ed->cand_page - 1);
        return 1;
    }

    /* Backspace 退一個注音鍵;組字串空了才交給外面刪文件裡的字 */
    if (c == KEY_BS) {
        if (ed->comp_len > 0) {
            ed->comp[--ed->comp_len] = 0;
            comp_refresh(ed);
            return 1;
        }
        return 0;
    }

    /* ESC 取消組字 */
    if (c == KEY_ESC) {
        if (ed->comp_len == 0) return 0;
        comp_reset(ed);
        return 1;
    }

    /* 注音鍵 -> 累積。數字鍵走的就是這條。 */
    if (c >= 0x20 && c < 0x7F && ime_key_bopo((char)c) != NULL) {
        if (ed->comp_len < IME_MAX_KEYS) {
            ed->comp[ed->comp_len++] = (char)c;
            ed->comp[ed->comp_len] = 0;
            comp_refresh(ed);
        }
        return 1;
    }

    /* 不是注音鍵(例如空白、標點)。組字中的話吃掉,避免打斷組字;
     * 沒在組字就讓它照一般編輯鍵處理。 */
    if (ed->comp_len > 0 && c >= 0x20 && c < 0x7F)
        return 1;

    return 0;
}

int ed_key(editor *ed, const key_event *ev)
{
    uint8_t c = ev->code;
    int changed = 0;

    ed->msg[0] = 0;

    /* Fn + Space 切換中英 */
    if (c == ' ' && (ev->mods & KEY_M_FN)) {
        ed->mode = (ed->mode == ED_MODE_ASCII) ? ED_MODE_BOPO : ED_MODE_ASCII;
        comp_reset(ed);
        set_msg(ed, ed->mode == ED_MODE_BOPO ? "注音" : "英數");
        ed->redraw = 1;
        return 1;
    }

    /* Fn + S 存檔 */
    if ((c == 's' || c == 'S') && (ev->mods & KEY_M_FN)) {
        ed->need_save = 1;
        return 1;
    }

    /* 組字中優先給注音處理 */
    if (ed->mode == ED_MODE_BOPO) {
        if (handle_bopo(ed, ev)) { ed->redraw = 1; return 1; }
    }

    switch (c) {
    case KEY_LEFT:  tb_left(&ed->tb);  changed = 1; break;
    case KEY_RIGHT: tb_right(&ed->tb); changed = 1; break;
    case KEY_UP: {
        size_t ls = tb_line_start(&ed->tb, tb_cursor(&ed->tb));
        if (ls > 0) {
            size_t prev = tb_line_start(&ed->tb, ls - 1);
            size_t off  = tb_cursor(&ed->tb) - ls;
            size_t end  = tb_line_end(&ed->tb, prev);
            tb_move_to(&ed->tb, (prev + off > end) ? end : prev + off);
        }
        changed = 1;
        break;
    }
    case KEY_DOWN: {
        size_t ls  = tb_line_start(&ed->tb, tb_cursor(&ed->tb));
        size_t end = tb_line_end(&ed->tb, tb_cursor(&ed->tb));
        if (end < tb_len(&ed->tb)) {
            size_t next = end + 1;
            size_t off  = tb_cursor(&ed->tb) - ls;
            size_t nend = tb_line_end(&ed->tb, next);
            tb_move_to(&ed->tb, (next + off > nend) ? nend : next + off);
        }
        changed = 1;
        break;
    }
    case KEY_BS:    tb_backspace(&ed->tb); changed = 1; break;
    case KEY_DEL:   tb_delete(&ed->tb);    changed = 1; break;
    case KEY_ENTER: tb_insert(&ed->tb, "\n", 1); changed = 1; break;
    default:
        if (c >= 0x20 && c < 0x7F) {
            char ch = (char)c;
            if (tb_insert(&ed->tb, &ch, 1) != 0) set_msg(ed, "緩衝區滿了");
            changed = 1;
        }
        break;
    }

    if (changed) { ed_reflow(ed); ed->redraw = 1; }
    return changed;
}
