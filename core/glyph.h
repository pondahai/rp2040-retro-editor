/* glyph - 從 Cubic 11 字型表取出一格 8x16 的點陣。
 *
 * 純 C、不碰硬體，所以查表邏輯(二分搜尋、缺字、寬字的左右半)可以在 PC 上測。
 *
 * 字型是 vendor/font_cjk.h,由 rp2040-ili9341-infones 的
 * tools/make_cjk_font.py 產生:Cubic 11(俐方體十一號)11x11 點陣字,
 * 已無損 repack 成 1 bit/pixel。7701 個寬字 + 95 個 ASCII,約 200KB。
 *
 * 座標系統:畫面切成 8 寬 x 16 高的格子。
 *   ASCII    佔 1 格
 *   中文/全形 佔 2 格(左半 LEAD、右半 TRAIL)
 */
/* 注意: 不能用 GLYPH_H 當 guard —— 下面拿它當格子高度,
 * 跟 lcd.h 的 LCD_H 是同一個坑。 */
#ifndef CORE_GLYPH_H
#define CORE_GLYPH_H

#include <stdint.h>

#define GLYPH_W      8       /* 一格的寬度(像素) */
#define GLYPH_H      16      /* 一格的高度(像素) */

typedef enum {
    GL_ASCII = 0,
    GL_WIDE_LEAD,            /* 寬字的左半 */
    GL_WIDE_TRAIL,           /* 寬字的右半 */
    GL_TOFU_LEAD,            /* 字型裡沒有這個字 -> 畫豆腐(空心框) */
    GL_TOFU_TRAIL
} glyph_kind;

typedef struct {
    glyph_kind kind;
    int        index;        /* 在字型表裡的位置，豆腐時無意義 */
} glyph_ref;

/* 一個 Unicode 碼位 -> 字型表位置。找不到會回豆腐。 */
glyph_ref glyph_find(uint32_t cp);

/* 取這一格第 row 列(0..GLYPH_H-1)的 8 個像素。
 * bit 0 = 最左邊 —— 跟 infones menu.cpp 的 `slice & 1; slice >>= 1` 同序。 */
uint8_t   glyph_slice(glyph_ref g, int row);

/* 這個碼位要佔幾格(1 或 2)。 */
int       glyph_cells(uint32_t cp);

/* UTF-8 解碼:讀一個碼位,回傳這個序列的 byte 數。非法序列回 1 且 *cp = 0xFFFD。 */
int       utf8_decode(const char *s, int len, uint32_t *cp);

#endif /* CORE_GLYPH_H */
