/* core/glyph.c 的 PC 端測試。
 *
 * 重點不是「字好不好看」,而是那些一改就會整片壞掉、但看起來又很合理的
 * 位元順序約定:左右半怎麼分、bit 0 在哪邊、yshift 有沒有算進去。
 */
#include "glyph.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); fails++; } \
} while (0)

/* 這個字在字型裡總共點亮幾個像素(兩個半邊加起來) */
static int ink(uint32_t cp)
{
    glyph_ref g = glyph_find(cp);
    int n = 0;
    for (int row = 0; row < GLYPH_H; row++) {
        uint8_t a = glyph_slice(g, row);
        glyph_ref t = g;
        if (t.kind == GL_WIDE_LEAD) t.kind = GL_WIDE_TRAIL;
        uint8_t b = (g.kind == GL_WIDE_LEAD) ? glyph_slice(t, row) : 0;
        for (int i = 0; i < 8; i++) { n += (a >> i) & 1; n += (b >> i) & 1; }
    }
    return n;
}

static void t_ascii(void)
{
    printf("ASCII\n");
    glyph_ref g = glyph_find('A');
    CHECK(g.kind == GL_ASCII, "'A' 是 ASCII");
    CHECK(glyph_cells('A') == 1, "ASCII 佔一格");
    CHECK(ink('A') > 10, "'A' 有筆畫");
    CHECK(ink(' ') == 0, "空白沒有筆畫");
}

static void t_cjk(void)
{
    printf("中文\n");
    CHECK(glyph_find(0x4E00).kind == GL_WIDE_LEAD, "「一」在字型裡");
    CHECK(glyph_cells(0x4E00) == 2, "中文佔兩格");
    CHECK(ink(0x4E00) > 5,  "「一」有筆畫");
    CHECK(ink(0x6F22) > 20, "「漢」筆畫比「一」多");
    CHECK(ink(0x6F22) > ink(0x4E00), "「漢」比「一」黑");
}

static void t_bopomofo(void)
{
    /* 碼表裡有注音符號本身,候選列才畫得出 ㄅㄆㄇ 而不是豆腐 */
    printf("注音符號\n");
    CHECK(glyph_find(0x3105).kind == GL_WIDE_LEAD, "ㄅ 在字型裡");
    CHECK(glyph_find(0x3129).kind == GL_WIDE_LEAD, "ㄩ 在字型裡");
    CHECK(ink(0x3105) > 5, "ㄅ 有筆畫");
}

static void t_tofu(void)
{
    printf("缺字畫豆腐\n");
    glyph_ref g = glyph_find(0x2A700);        /* 擴充 B 區,字型裡沒有 */
    CHECK(g.kind == GL_TOFU_LEAD, "缺字回豆腐");
    CHECK(glyph_slice(g, 2) == 0xFF, "豆腐上緣是實線");
    CHECK(glyph_slice(g, 7) == 0x01, "左半只有最左邊那一直條");
    g.kind = GL_TOFU_TRAIL;
    CHECK(glyph_slice(g, 7) == 0x80, "右半只有最右邊那一直條");
}

static void t_halves_differ(void)
{
    /* 左右半如果拿成同一個 byte,畫面上會是左右對稱的假字 —— 
     * 看起來像字,但每個字都對稱,很難第一眼看出是 bug */
    printf("左右半不能相同\n");
    glyph_ref lead = glyph_find(0x6F22);      /* 漢 */
    glyph_ref trail = lead;
    trail.kind = GL_WIDE_TRAIL;
    int differ = 0;
    for (int row = 0; row < GLYPH_H; row++)
        if (glyph_slice(lead, row) != glyph_slice(trail, row)) differ = 1;
    CHECK(differ, "「漢」的左右半不一樣");
}

static void t_yshift(void)
{
    /* yshift = 2:第 0、1 列一定是空的,第 15 列也是(13 列 + 2 位移 = 15) */
    printf("yshift\n");
    glyph_ref g = glyph_find(0x6F22);
    CHECK(glyph_slice(g, 0) == 0, "第 0 列是空的");
    CHECK(glyph_slice(g, 1) == 0, "第 1 列是空的");
    CHECK(glyph_slice(g, 15) == 0, "第 15 列是空的");
}

static void t_utf8(void)
{
    uint32_t cp;
    printf("UTF-8 解碼\n");
    CHECK(utf8_decode("A", 1, &cp) == 1 && cp == 'A', "ASCII");
    CHECK(utf8_decode("\xE6\xBC\xA2", 3, &cp) == 3 && cp == 0x6F22, "「漢」");
    CHECK(utf8_decode("\xE6\xBC", 2, &cp) == 1 && cp == 0xFFFD, "截斷的序列");
    CHECK(utf8_decode("\xFF", 1, &cp) == 1 && cp == 0xFFFD, "非法首位元組");
}

int main(void)
{
    t_ascii();
    t_cjk();
    t_bopomofo();
    t_tofu();
    t_halves_differ();
    t_yshift();
    t_utf8();

    if (fails == 0) { printf("\n全部通過\n"); return 0; }
    printf("\n%d 項失敗\n", fails);
    return 1;
}
