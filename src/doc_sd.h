/* 文件的 SD 卡存取。
 *
 * ⚠ 目前是 stub，還沒接上 FatFs —— 每個函式都回 DOC_ENOIMPL。
 *
 * 為什麼先留空：生態系現有的韌體(infones / PicoApple2 / doom / loader)
 * 全部只「讀」SD 卡。編輯器是第一個要「寫」的，掉電損毀、f_sync 的時機、
 * FF_FS_READONLY=0 之後多出來的 code size,這些都還沒定案。與其先塞一版
 * 半吊子的寫入進去,不如把介面定好、其餘先擋住。
 *
 * 接上去的時候要搬的東西在
 *     rp2040-ili9341-infones/software/infones/drivers/{fatfs,sdcard}
 * 但注意那是唯讀用法,要自己打開寫入。
 */
#ifndef DOC_SD_H
#define DOC_SD_H

#include <stddef.h>

#define DOC_OK        0
#define DOC_ENOIMPL  -1      /* 還沒實作 */
#define DOC_EIO      -2
#define DOC_ENOENT   -3
#define DOC_ETOOBIG  -4

int doc_sd_init(void);

/* 讀進 buf，回傳 bytes(>=0)或負的錯誤碼。 */
int doc_sd_load(const char *name, char *buf, size_t cap);

/* 寫出去。回傳 DOC_OK 或負的錯誤碼。 */
int doc_sd_save(const char *name, const char *data, size_t len);

#endif /* DOC_SD_H */
