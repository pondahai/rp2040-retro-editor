/* 文件的 SD 卡存取。
 *
 * 底層是 FatFs + SD 驅動,整套搬自 rp2040-ili9341-infones 的
 * drivers/{fatfs,sdcard}。接腳預設就是 spi1 + 13/10/11/12,跟 board.h 一致。
 *
 * ⚠ 這是生態系裡**第一個會寫 SD 卡的韌體**。其他專案(infones / PicoApple2 /
 * doom / loader)都只讀。所以存檔走的是「寫暫存檔再換名」,細節見 doc_sd.c。
 */
#ifndef DOC_SD_H
#define DOC_SD_H

#include <stddef.h>

#define DOC_OK        0
#define DOC_ENOCARD  -1      /* 掛載失敗:沒插卡、或不是 FAT */
#define DOC_EIO      -2
#define DOC_ENOENT   -3
#define DOC_ETOOBIG  -4      /* 檔案比緩衝區大 */

/* 掛載 SD 卡。回 DOC_OK 或 DOC_ENOCARD。 */
int doc_sd_init(void);

/* 讀進 buf。回傳 bytes(>=0)或負的錯誤碼。 */
int doc_sd_load(const char *name, char *buf, size_t cap);

/* 寫出去。回 DOC_OK 或負的錯誤碼。 */
int doc_sd_save(const char *name, const char *data, size_t len);

/* 錯誤碼 -> 給使用者看的短字串(狀態列只有一行)。 */
const char *doc_sd_strerror(int err);

#endif /* DOC_SD_H */
