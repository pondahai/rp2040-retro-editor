/* 鍵盤矩陣的硬體層 —— 只負責「把 8x8 的按住狀態讀出來」。
 *
 * 去彈跳、修飾鍵、連發那些真正容易寫錯的邏輯全部在 vendor/keys.c,
 * 那一層是純 C、不碰 GPIO,可以在 PC 上測。這裡刻意只留掃描時序。
 */
#ifndef HW_KEYS_H
#define HW_KEYS_H

#include <stdint.h>
#include "keys.h"

void hw_keys_init(void);

/* 掃一次,把結果交給 vendor/keys.c 轉成事件。
 * 回傳事件數(0..max)。 */
int  hw_keys_poll(keys *k, key_event *out, int max);

#endif /* HW_KEYS_H */
