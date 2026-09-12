/* 8 顆直接接 GPIO 的遊戲按鍵 -> 跟鍵盤矩陣同一種 key_event。
 *
 * 為什麼編輯器要用 D-pad:鍵盤硬體**已經把方向鍵拿掉了**,矩陣上
 * UP/DOWN/LEFT/RIGHT/PGUP/PGDN 那幾格現在空著。游標移動與候選字瀏覽
 * 因此改由這 8 顆負責。
 *
 * 這一套跟矩陣是完全獨立的硬體(見 rp2040-retro-handheld/docs/HARDWARE.md):
 * active-low、內部上拉、直接 gpio_get(),不經過 74HC165。
 *
 * 去彈跳與連發的參數跟 vendor/keys.c 一致,不然同一個畫面上兩種按鍵的
 * 手感會不一樣。
 */
#ifndef HW_DPAD_H
#define HW_DPAD_H

#include <stdint.h>
#include "keys.h"

void hw_dpad_init(void);

/* 掃一次,把按鍵轉成事件接在 out 後面。
 * n 是 out 裡已經有的事件數,回傳新的總數。 */
int  hw_dpad_poll(int n, key_event *out, int max);

#endif /* HW_DPAD_H */
