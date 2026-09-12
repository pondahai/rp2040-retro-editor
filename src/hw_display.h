/* 把 editor 的狀態畫到 ILI9341 上。
 *
 * 底層是 vendor/lcd.c 的文字模式(40x30 格、8x8 字、blocking、沒有
 * framebuffer)。版面：
 *
 *     row 0        狀態列：檔名、模式、dirty 標記、訊息
 *     row 1..28    內文
 *     row 29       注音組字與候選字
 *
 * ⚠ 目前只有 ASCII 畫得出來。中文字會畫成一個實心方塊佔位 ——
 *   字型還沒接上(見 README 的「還沒做」)。
 */
#ifndef HW_DISPLAY_H
#define HW_DISPLAY_H

#include "editor.h"

void hw_display_init(void);
void hw_display_draw(const editor *ed);

#endif /* HW_DISPLAY_H */
