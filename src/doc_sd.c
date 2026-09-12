#include "doc_sd.h"

/* 見 doc_sd.h：這一層還沒接 FatFs。
 *
 * 刻意不做「假裝成功」的 stub —— 那會讓使用者以為存檔了。回明確的錯誤碼，
 * 主迴圈會把它顯示在狀態列上。 */

int doc_sd_init(void)
{
    return DOC_ENOIMPL;
}

int doc_sd_load(const char *name, char *buf, size_t cap)
{
    (void)name; (void)buf; (void)cap;
    return DOC_ENOIMPL;
}

int doc_sd_save(const char *name, const char *data, size_t len)
{
    (void)name; (void)data; (void)len;
    return DOC_ENOIMPL;
}
