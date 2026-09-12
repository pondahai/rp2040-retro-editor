#include "glyph.h"
#include "font_cjk.h"

/* 豆腐:空心框。字型裡沒有的字要畫得出「這裡有一個字但我不認得」,
 * 不然使用者只會看到消失的字元,以為是自己打錯。
 * 左半畫左邊框與上下,右半畫右邊框與上下。 */
static uint8_t tofu_slice(int lead, int row)
{
    if (row < 2 || row > 13) return 0x00;
    if (row == 2 || row == 13) return 0xFF;
    return lead ? 0x01 : 0x80;
}

int glyph_cells(uint32_t cp)
{
    return (cp >= 0x20 && cp < 0x7F) ? 1 : 2;
}

glyph_ref glyph_find(uint32_t cp)
{
    glyph_ref g;

    if (cp >= ASCII_FIRST && cp < (uint32_t)(ASCII_FIRST + ASCII_COUNT)) {
        g.kind = GL_ASCII;
        g.index = (int)(cp - ASCII_FIRST);
        return g;
    }

    /* cjk_index 是排序過的 uint16,二分搜尋 */
    if (cp <= 0xFFFF) {
        int lo = 0, hi = CJK_GLYPH_COUNT - 1;
        uint16_t want = (uint16_t)cp;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            uint16_t probe = cjk_index[mid];
            if (probe == want) {
                g.kind = GL_WIDE_LEAD;
                g.index = mid;
                return g;
            }
            if (probe < want) lo = mid + 1;
            else hi = mid - 1;
        }
    }

    g.kind = GL_TOFU_LEAD;
    g.index = 0;
    return g;
}

uint8_t glyph_slice(glyph_ref g, int row)
{
    if (row < 0 || row >= GLYPH_H) return 0;

    switch (g.kind) {
    case GL_ASCII: {
        /* ASCII 的表沒有 yshift,直接對到格子的上緣 */
        if (row >= ASCII_GLYPH_ROWS) return 0;
        return ascii_bitmap[g.index * ASCII_GLYPH_ROWS + row];
    }
    case GL_WIDE_LEAD:
    case GL_WIDE_TRAIL: {
        int gr = row - CJK_GLYPH_YSHIFT;
        uint16_t bits;
        if (gr < 0 || gr >= CJK_GLYPH_ROWS) return 0;
        bits = cjk_bitmap[g.index * CJK_GLYPH_ROWS + gr];
        /* 跟 infones menu.cpp 一致:低位元組是左半,高位元組是右半 */
        return (uint8_t)((g.kind == GL_WIDE_LEAD) ? (bits & 0xff) : (bits >> 8));
    }
    case GL_TOFU_LEAD:  return tofu_slice(1, row);
    case GL_TOFU_TRAIL: return tofu_slice(0, row);
    }
    return 0;
}

int utf8_decode(const char *s, int len, uint32_t *cp)
{
    unsigned char c;
    int n, i;
    uint32_t v;

    if (len <= 0) { *cp = 0; return 0; }
    c = (unsigned char)s[0];

    if (c < 0x80)            { *cp = c; return 1; }
    else if ((c & 0xE0) == 0xC0) { n = 2; v = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { n = 3; v = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { n = 4; v = c & 0x07; }
    else { *cp = 0xFFFD; return 1; }

    if (n > len) { *cp = 0xFFFD; return 1; }
    for (i = 1; i < n; i++) {
        unsigned char b = (unsigned char)s[i];
        if ((b & 0xC0) != 0x80) { *cp = 0xFFFD; return 1; }
        v = (v << 6) | (b & 0x3F);
    }
    *cp = v;
    return n;
}
