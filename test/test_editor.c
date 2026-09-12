/* core/editor.c 的 PC 端測試。
 *
 * 第 4 組是真機上回報的第一個 bug 的回歸測試:注音模式下數字鍵變成數字。
 * 原因是本來拿 1-9 當選字鍵,但大千配列的數字鍵本身就是注音
 * (1=ㄅ 2=ㄉ 5=ㄓ 8=ㄚ 9=ㄞ 0=ㄢ,3/4/6/7 是聲調)。
 * 選字改成 Enter / 上下 / PgUp / PgDn,跟 rp2040-retro-dict 一致。
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
    CHECK(strcmp(buf, "1") == 0, "英數模式打 1 得到 \"1\"");

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
            key('s', 0);                     /* 打一個 ㄋ,製造候選 */
            dump_text(before, sizeof before);
            key(d, 0);
            dump_text(after, sizeof after);
            if (strcmp(before, after) != 0) {
                printf("  FAIL 有候選時按 '%c' 插進了文件\n", d);
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
        printf("   組字串 = \"%s\"  注音 = \"%s\"  候選數 = %d\n",
               ed.comp, bopo, ed.cand_count);
        /* ㄋㄧˇ = E3 84 8B  E3 84 A7  CB 87 */
        CHECK(strcmp(bopo, "\xE3\x84\x8B\xE3\x84\xA7\xCB\x87") == 0,
              "su3 -> ㄋㄧˇ");
    }
    CHECK(ed.cand_count > 0, "查得到候選字");

    printf("6. Enter 選字\n");
    key(KEY_ENTER, 0);
    dump_text(buf, sizeof buf);
    printf("   文件內容 = \"%s\"\n", buf);
    CHECK(strlen(buf) > 1, "選到字了");
    CHECK(ed.comp_len == 0, "選完字組字串清空");

    printf("7. 上下鍵移動候選\n");
    key('s', 0); key('u', 0); key('3', 0);
    CHECK(ed_cand_sel(&ed) == 0, "一開始選第一個");
    key(KEY_DOWN, 0);
    CHECK(ed_cand_sel(&ed) == 1, "往下移一個");
    key(KEY_UP, 0);
    CHECK(ed_cand_sel(&ed) == 0, "往上移回來");
    key(KEY_UP, 0);
    CHECK(ed_cand_sel(&ed) == 0, "在第一個時往上不會變負的");

    printf("8. PgDn 翻頁\n");
    if (ed.cand_count > ED_CAND_MAX) {
        key(KEY_PGDN, 0);
        CHECK(ed.cand_page == 1, "翻到第二頁");
        CHECK(ed_cand_sel(&ed) == 0, "翻頁後選回第一個");
    }

    printf("9. Esc 取消組字\n");
    key(KEY_ESC, 0);
    CHECK(ed.comp_len == 0, "組字串清空");
    CHECK(ed_cand_count(&ed) == 0, "候選清空");

    printf("10. 切回英數,數字鍵恢復成數字\n");
    key(' ', KEY_M_FN);
    CHECK(ed.mode == ED_MODE_ASCII, "切回英數");
    {
        char before[256], after[256];
        dump_text(before, sizeof before);
        key('7', 0);
        dump_text(after, sizeof after);
        CHECK(strlen(after) == strlen(before) + 1, "英數模式打 7 會插入 7");
    }

    if (fails == 0) { printf("\n全部通過\n"); return 0; }
    printf("\n%d 項失敗\n", fails);
    return 1;
}
