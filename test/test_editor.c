/* core/editor.c 的 PC 端測試。
 *
 * 兩組回歸測試對應真機上回報的 bug:
 *
 *   第 4 組 注音模式下數字鍵變成數字。原本拿 1-9 當選字鍵,但大千配列的
 *          數字鍵本身就是注音(1=ㄅ 2=ㄉ 5=ㄓ 8=ㄚ 9=ㄞ 0=ㄢ,3/4/6/7 是聲調)。
 *   第 8 組 硬體已經拿掉方向鍵,所以游標移動與選字都不能依賴它們。
 */
#include "editor.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); fails++; } \
} while (0)

static char storage[4096];
static editor ed;

static void key(uint8_t code, uint8_t mods)
{
    key_event ev; ev.code = code; ev.mods = mods; ev.repeat = 0;
    ed_key(&ed, &ev);
}

static void dump_text(char *out, size_t n)
{
    size_t got = tb_copy(&ed.tb, 0, out, n - 1);
    out[got] = 0;
}

int main(void)
{
    char buf[256];

    ed_init(&ed, storage, sizeof storage, "t.txt");

    printf("1. 一開始是英數\n");
    CHECK(ed.mode == ED_MODE_ASCII, "預設英數");
    key('1', 0);
    dump_text(buf, sizeof buf);
    CHECK(strcmp(buf, "1") == 0, "英數模式打 1 得到 1");

    printf("2. Fn+Space 切到注音\n");
    key(' ', KEY_M_FN);
    CHECK(ed.mode == ED_MODE_BOPO, "切到注音了");

    printf("3. 注音模式下數字鍵是注音鍵\n");
    key('1', 0);
    dump_text(buf, sizeof buf);
    CHECK(ed.comp_len == 1, "組字串累積了一個鍵");
    CHECK(strcmp(buf, "1") == 0, "文件沒有被插入第二個 1");

    printf("4. 回歸:有候選時,每個數字鍵都不能掉進文件\n");
    {
        char before[256], after[256];
        char d;
        for (d = '0'; d <= '9'; d++) {
            ed.comp_len = 0; ed.comp[0] = 0;
            key('s', 0);
            dump_text(before, sizeof before);
            key(d, 0);
            dump_text(after, sizeof after);
            if (strcmp(before, after) != 0) {
                printf("  FAIL 有候選時按 %c 插進了文件\n", d);
                fails++;
            }
        }
        ed.comp_len = 0; ed.comp[0] = 0;
    }

    printf("5. 打完整音節 su3\n");
    ed.comp_len = 0; ed.comp[0] = 0;
    key('s', 0); key('u', 0); key('3', 0);
    {
        char bopo[64];
        int n = ime_bopomofo(ed.comp, bopo, sizeof bopo);
        bopo[n] = 0;
        printf("   組字串 = %s  注音 = %s  候選數 = %d\n",
               ed.comp, bopo, ed.cand_count);
        CHECK(strcmp(bopo, "\xE3\x84\x8B\xE3\x84\xA7\xCB\x87") == 0,
              "su3 -> ㄋㄧˇ");
    }
    CHECK(ed.cand_count > 0, "查得到候選字");

    printf("6. Fn+1 選第一個候選\n");
    key(KEY_F1, KEY_M_FN);
    dump_text(buf, sizeof buf);
    printf("   文件內容 = %s\n", buf);
    CHECK(strlen(buf) > 1, "選到字了");
    CHECK(ed.comp_len == 0, "選完字組字串清空");

    printf("7. Space 移動反白,Enter 確認\n");
    key('s', 0); key('u', 0); key('3', 0);
    CHECK(ed_cand_sel(&ed) == 0, "一開始選第一個");
    key(' ', 0);
    CHECK(ed_cand_sel(&ed) == 1, "Space 往下移一個");
    key(' ', 0);
    CHECK(ed_cand_sel(&ed) == 2, "再移一個");
    {
        size_t before = tb_len(&ed.tb);
        key(KEY_ENTER, 0);
        CHECK(tb_len(&ed.tb) > before, "Enter 選到字");
    }

    printf("8. 回歸:硬體無方向鍵,用 Fn+IJKL\n");
    {
        size_t cur, len;
        key(' ', KEY_M_FN);
        CHECK(ed.mode == ED_MODE_ASCII, "切回英數");
        key('a', 0); key('b', 0); key('c', 0);
        cur = tb_cursor(&ed.tb);

        key('j', KEY_M_FN);
        CHECK(tb_cursor(&ed.tb) < cur, "Fn+J 游標往左");
        key('l', KEY_M_FN);
        CHECK(tb_cursor(&ed.tb) == cur, "Fn+L 游標往右");

        len = tb_len(&ed.tb);
        key('j', 0);
        CHECK(tb_len(&ed.tb) == len + 1, "單獨按 j 是插入字母");
        key(KEY_BS, 0);
    }

    printf("9. Esc 取消組字\n");
    key(' ', KEY_M_FN);
    key('s', 0); key('u', 0);
    CHECK(ed.comp_len == 2, "組字中");
    key(KEY_ESC, 0);
    CHECK(ed.comp_len == 0, "組字串清空");
    CHECK(ed_cand_count(&ed) == 0, "候選清空");

    printf("10. 切回英數,數字鍵恢復成數字\n");
    key(' ', KEY_M_FN);
    CHECK(ed.mode == ED_MODE_ASCII, "切回英數");
    {
        size_t before = tb_len(&ed.tb);
        key('7', 0);
        CHECK(tb_len(&ed.tb) == before + 1, "英數模式打 7 會插入 7");
    }

    if (fails == 0) { printf("\n全部通過\n"); return 0; }
    printf("\n%d 項失敗\n", fails);
    return 1;
}
