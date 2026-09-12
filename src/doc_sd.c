#include "doc_sd.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

static FATFS fs;
static int   mounted = 0;

int doc_sd_init(void)
{
    FRESULT r = f_mount(&fs, "", 1);   /* 1 = 立刻掛載,不要延後到第一次開檔 */
    mounted = (r == FR_OK);
    return mounted ? DOC_OK : DOC_ENOCARD;
}

int doc_sd_load(const char *name, char *buf, size_t cap)
{
    FIL f;
    FRESULT r;
    UINT got = 0;
    FSIZE_t size;

    if (!mounted) return DOC_ENOCARD;

    r = f_open(&f, name, FA_READ);
    if (r == FR_NO_FILE || r == FR_NO_PATH) return DOC_ENOENT;
    if (r != FR_OK) return DOC_EIO;

    size = f_size(&f);
    if (size > cap) { f_close(&f); return DOC_ETOOBIG; }

    r = f_read(&f, buf, (UINT)size, &got);
    f_close(&f);

    if (r != FR_OK) return DOC_EIO;
    return (int)got;
}

/*
 * 存檔:先寫暫存檔,確定整份都落地了才換名。
 *
 * 直接對原檔 f_open(FA_CREATE_ALWAYS) 的話,那一瞬間原檔就被截成 0 了 ——
 * 掉電或拔卡的話兩份都沒有。掌機沒有電池監測、使用者隨時可能直接拔電,
 * 這個視窗不能留。
 *
 * 換名之前那次 f_sync 不是多餘的:f_close 會 flush,但**先確定資料真的寫進
 * 卡裡、再去動目錄項**,順序才是對的。這樣掉電的結果只有三種:
 *
 *   1. 還沒換名 -> 原檔完好,多一個 .TMP
 *   2. 換名途中 -> FAT 的 rename 是單一目錄項操作,不會兩個都壞
 *   3. 換好了   -> 新檔完好
 *
 * 代價是存檔期間卡上會短暫存在兩份完整的文件。
 */
int doc_sd_save(const char *name, const char *data, size_t len)
{
    FIL f;
    FRESULT r;
    UINT wrote = 0;
    char tmp[64];

    if (!mounted) return DOC_ENOCARD;

    snprintf(tmp, sizeof tmp, "~%.*s", (int)(sizeof tmp - 8), name);

    r = f_open(&f, tmp, FA_WRITE | FA_CREATE_ALWAYS);
    if (r != FR_OK) return DOC_EIO;

    if (len > 0) {
        r = f_write(&f, data, (UINT)len, &wrote);
        if (r != FR_OK || wrote != len) { f_close(&f); f_unlink(tmp); return DOC_EIO; }
    }

    r = f_sync(&f);                     /* 先落地 */
    f_close(&f);
    if (r != FR_OK) { f_unlink(tmp); return DOC_EIO; }

    f_unlink(name);                     /* 舊檔可能不存在,失敗不算錯 */
    r = f_rename(tmp, name);            /* 再動目錄項 */
    if (r != FR_OK) { f_unlink(tmp); return DOC_EIO; }

    return DOC_OK;
}

const char *doc_sd_strerror(int err)
{
    switch (err) {
    case DOC_OK:       return "OK";
    case DOC_ENOCARD:  return "no SD card";
    case DOC_ENOENT:   return "new file";
    case DOC_ETOOBIG:  return "file too big";
    case DOC_EIO:      return "SD write failed";
    }
    return "?";
}
