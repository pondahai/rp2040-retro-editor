# rp2040-retro-editor

RP2040 掌機上的**注音中文文件編輯器**，文件存在 SD 卡上。

同時是 [`rp2040-retro-loader`](https://github.com/pondahai/rp2040-retro-loader)
的 **app 範本** —— 要寫新的掌機韌體，可以從這份抄。

> 🚧 **開發中**。LCD、鍵盤矩陣、注音組字、編輯核心、中文字型、SD 卡存取
> 都通了。**尚未在真機上跑過**（見下方「還沒做」）。

---

## 這份為什麼可以當範本

`rp2040-retro-loader` 對「放上 SD 卡的韌體」有五條必修規則。這個專案全部做齊，
而且刻意**不**做選修的第六條（flash 資料區位移）—— 文件存 SD、不佔 flash，
所以用不到。要抄的話，這是最小的完整範例。

| # | 必修 | 這裡怎麼做 |
| :-- | :--- | :--- |
| 1 | link 到 `0x10004000` | `CMakeLists.txt` 的 `LOADER_OFFSET_BUILD` 選項，換成 loader 的 `app/memmap_app.ld` |
| 2 | 不帶 `.boot2`、向量表在最前面 | 同上那份 ld 負責。已用 `objdump` 驗證 |
| 3 | 產出兩個 uf2 | `retro_editor.uf2`（只能放 SD）與 `retro_editor_standalone.uf2`（跳板已合併） |
| 4 | `.uf2` 放 SD 根目錄 | 部署時的事，不支援子目錄 |
| 5 | 知道重置的語意被改掉了 | 見下方「⚠️ 重置」 |
| 6 | 配一張 96×96 封面 | `RETRO_EDITOR.RAW`，跟 uf2 同名同放根目錄 |

### 驗證紀錄

偏移編譯出來的 `retro_editor.elf`：

```
.text  VMA 0x10004000          # 必修 1
沒有 .boot2 段                  # 必修 2
向量表 SP=0x20042000  Reset=0x100040f7（Thumb bit 已設）
```

正好符合載入器 `app_present()` 的檢查條件（SP 落在 SRAM、進入點落在 APP 區
且是 Thumb）。跟 loader README 記錄的最小測試專案數值一致。

---

## 分層

這個分法是從 `rp2040-retro-dict` 學來的：**真正容易寫錯的邏輯，全部放在能寫
測試的那一側**。

```
core/     純 C，不碰硬體，PC 上可測
          textbuf.c   gap buffer，UTF-8 邊界安全
          editor.c    編輯器狀態機 + 注音組字狀態
          glyph.c     字型查表（二分搜尋、寬字左右半、缺字豆腐）
vendor/   從生態系其他專案搬來、已在真機驗證過的
          lcd.c       ILI9341 驅動（搬自 loader，拿掉選單背景牆）
          font_cjk.h  Cubic 11 中文字型（搬自 infones，見下方）
          keys.c      去彈跳 / 修飾鍵 / 連發（搬自 retro-dict）
          ime.c       注音查碼（搬自 retro-dict）
          drivers/    FatFs + SD 驅動（搬自 infones，整套未改）
src/      只有這一層碰 GPIO
          hw_keys.c   74HC595/165 掃描時序
          hw_dpad.c   8 鍵 D-pad（方向鍵，矩陣上已經沒有了）
          hw_display.c  版面
          doc_sd.c    SD 存取（寫暫存檔再換名）
          board.h     接腳
```

`core/` 完全不依賴 pico-sdk，所以：

```bash
test/build_test.bat     # 需要 VS2022 Community
```

兩組測試。`textbuf` 那 10 組全部用中文而不是 `"abc"` —— 這一層會出錯的地方
都在多位元組邊界上（backspace 刪一個字而不是一個 byte、游標不能停在中文字
中間）。`glyph` 那組驗的是位元順序的約定：左右半不能拿成同一個 byte（會畫出
左右對稱、看起來像字的假字）、yshift 有沒有算進去、缺字要畫豆腐。

---

## 建置

### 一般編譯（USB BOOTSEL 直燒）

```bash
cmake -S . -B build -G Ninja -DPICO_BOARD=pico
ninja -C build
# -> build/retro_editor.uf2
```

### 偏移編譯（給載入器用，放 SD 卡）

```bash
cmake -S . -B build_offset -G Ninja -DPICO_BOARD=pico -DLOADER_OFFSET_BUILD=ON
ninja -C build_offset
# -> build_offset/retro_editor.uf2             只能放 SD，不能單獨燒
# -> build_offset/retro_editor_standalone.uf2  兩種都可以
```

預設假設 `rp2040-retro-loader` 跟這個倉庫並排。不是的話用
`-DLOADER_PATH=<路徑>` 指定。`_standalone` 版需要 loader 先 build 過一次
（要它的 `build/trampoline.uf2`）。

> 💡 **cmake 抱怨找不到 picotool 或 pioasm** 的話，把這兩個指到 VS Code
> 擴充裝好的那份，就不必在命令列裡準備 host 編譯器（`pioasm` 要用 host
> C++ 才能把 `.pio` 組成 header）：
>
> ```
> -Dpicotool_DIR=<你的>/.pico-sdk/picotool/2.2.0-a4/picotool
> -Dpioasm_DIR=<你的>/.pico-sdk/tools/2.2.0/pioasm
> ```

---

## 操作

這台機器**兩套輸入並存**，編輯器兩套都用：鍵盤矩陣負責打字，**D-pad 負責方向**。

> 📌 **為什麼方向要走 D-pad**：鍵盤硬體已經把方向鍵拿掉了，矩陣上
> `UP/DOWN/LEFT/RIGHT/PGUP/PGDN` 那幾格現在空著。這 8 顆直接接 GPIO 的遊戲
> 按鍵（`UP 9 / DOWN 5 / LEFT 8 / RIGHT 6 / A 2 / B 3 / SELECT 28 / START 4`）
> 是獨立的硬體，不經過 74HC165。

### D-pad

| 按鍵 | 英數模式 | 注音組字中 |
| :--- | :--- | :--- |
| `↑` `↓` | 游標上下移動 | 移動候選字（反白） |
| `←` `→` | 游標左右移動 | 游標左右移動 |
| `A` / `START` | 換行 | **選字** |
| `B` | — | 取消組字 |
| `SELECT` | — | 候選字翻頁 |

方向鍵按住不放會連發（400ms 起跳、60ms 間隔），跟矩陣的手感一致。

### 鍵盤

| 鍵 | 動作 |
| :--- | :--- |
| `Fn` + `Space` | 切換英數 / 注音 |
| `Fn` + `S` | 存檔 |
| `Fn` + `1`~`9` | **直接選第幾個候選**（候選列上標著數字） |
| `Fn` + `I` `J` `K` `L` | 游標上下左右（D-pad 的備援，倒 T 排列） |
| `Fn` + `U` `O` | 候選字上一頁 / 下一頁 |
| `Enter` | 選字 / 換行 |
| `Space` | 注音組字中：反白往下移一個，到底就翻頁 |
| `Backspace` | 退一個注音鍵；沒在組字就刪文件裡的字 |
| `Esc` | 取消組字 |
| 注音鍵 | 組字（大千配列，一聲不用打；`3/4/6/7` 是二/三/四/輕聲） |

> ⚠️ **選字不能直接按數字鍵。** 大千配列的數字鍵本身就是注音（`1`=ㄅ `2`=ㄉ
> `5`=ㄓ `8`=ㄚ `9`=ㄞ `0`=ㄢ，`3/4/6/7` 是聲調），拿來選字的話數字鍵就永遠
> 打不出注音——這是真機上回報的第一個 bug。所以要選第幾個得按 `Fn`+數字
> （`keys.c` 的 `fn_translate()` 會轉成 `F1`~`F9`，不跟注音衝突）。

輸入走 **8×8 鍵盤矩陣**（64 鍵實體 QWERTY）＋ **8 鍵 D-pad**。
接腳見 [`rp2040-retro-handheld/docs/HARDWARE.md`](https://github.com/pondahai/rp2040-retro-handheld/blob/main/docs/HARDWARE.md)。

### ⚠️ 重置（必修 5）

載入器改變了「重置」的意義：**軟重置會直接穿透跳回本韌體，不顯示選單**。
只有冷開機、按實體 RESET、或**開機時按住 B** 才會進選單。

交棒之後載入器就不存在了（它的 RAM 已經被本韌體拿去用），所以編輯器**沒有
辦法「回選單」**，只能重開機。

---

## 中文字型：Cubic 11

用的是 **Cubic 11（俐方體十一號）**，11×11 點陣字，繁體。不是向量字型縮出來的
——這個尺寸下原生點陣比縮放清楚得多，而且注音打出來的是繁體字。

字型表直接搬 `rp2040-ili9341-infones` 的 `font_cjk.h`（7,701 個寬字 + 95 個
ASCII，1 bit/pixel，約 200KB 進 flash）。它的源頭是
`pico_keyboard_ime_terminal_usb_host`，經由 `pondahai/ime-charset-font-bitmap`
的管線產生。

**不放 SD 卡**：編輯器不想為了顯示中文而依賴插著卡。`rp2040-retro-dict` 另有一套
16×16 2bit 灰階 Noto 放在 SD（`FONT.BIN`，1.25MB、14,516 字），畫質更好、字集更大，
但多一個依賴，這裡不採用。

格式上有三個一改就整片壞掉的約定，`core/glyph.c` 都有測試守著：

- 一格 **8 寬 × 16 高**，中文佔**兩格**；左半 = `bits & 0xff`，右半 = `bits >> 8`
- **bit 0 是最左邊**的像素
- `YSHIFT 2` 已烘進列位置，所以 `glyphRow = row - 2`

碼表裡**含注音符號本身**（`U+3105`–`U+3129` 就是 ㄅㄆㄇ），所以候選列畫的是
真的注音，不必另外找字。

## 封面圖示（必修 6）

載入器的選單是圖形化的，每個 uf2 可以配一張封面：

```bash
python ../rp2040-retro-loader/tools/make_thumb.py tools/icon.png -o RETRO_EDITOR.RAW
```

- **96×96 RGB565 big-endian，沒有標頭，剛好 18432 bytes**
- 檔名要跟 uf2 一致：`RETRO_EDITOR.UF2` 配 `RETRO_EDITOR.RAW`，兩個都放 SD 根目錄
- 載入器只用檔案長度擋「拖錯檔案」（沒有標頭可以驗），所以長度必須剛好
- 沒有的話不會怎樣，載入器會畫佔位圖

為什麼不是 PNG：解碼器就等於程式碼，而載入器全部只有 16KB。

## 還沒做

| 項目 | 現況 |
| :--- | :--- |
| ~~SD 卡存取~~ | ✅ 已接上，見下節 |
| **自動存檔** | 目前只有 `Fn+S` 手動存。掉電會丟掉未存的內容 |
| **開檔選單** | 檔名寫死 `NOTE.TXT`。沒有檔案瀏覽器 |
| ~~中文字型~~ | ✅ 已接上 Cubic 11，見下節 |
| **真機測試** | 尚未在板子上跑過。編譯與向量表驗證過了，畫面沒有 |

### SD 存取：生態系第一個會「寫」的韌體

其他專案（infones / PicoApple2 / doom / loader）**全部只讀 SD**。

FatFs 與 SD 驅動整套搬自 `rp2040-ili9341-infones` 的 `drivers/`，一行沒改
——它的 `ffconf.h` 本來就是 `FF_FS_READONLY 0`，`disk_write()` 也早就實作好了，
只是 infones 自己沒用到。接腳預設 spi1 + 13/10/11/12，跟 `board.h` 一致。
底層走 PIO SPI（`spi.pio`），不佔用硬體 spi1。

**存檔是寫暫存檔再換名**，不是直接覆蓋原檔：

```
寫 ~NOTE.TXT  ->  f_sync（確定落地）->  f_close
              ->  f_unlink(NOTE.TXT)  ->  f_rename
```

直接 `f_open(FA_CREATE_ALWAYS)` 的話，那一瞬間原檔就被截成 0 了——掉電或
拔卡就兩份都沒有。掌機沒有電池監測、使用者隨時可能直接拔電，這個視窗不能留。
換名前那次 `f_sync` 不是多餘的：**先確定資料真的進了卡，再去動目錄項**，
順序才是對的。掉電的結果只有三種：原檔完好多一個 `~` 暫存檔、rename 是單一
目錄項操作不會兩邊都壞、或新檔完好。

代價是存檔期間卡上會短暫存在兩份完整的文件。

存檔時不必另外開一塊 32KB 來拼接 gap buffer：**把游標移到最尾端，gap 就整塊
跑到後面去**，`buf[0..len)` 剛好是連續的文字，存完再把游標移回原位。

---

## 授權與出處

| 來源 | 授權 |
| :--- | :--- |
| `vendor/font_cjk.h` | Cubic 11（俐方體十一號），OFL；經 infones 的 `make_cjk_font.py` 重新打包 |
| `vendor/lcd.c` | 搬自 `rp2040-retro-loader` |
| `vendor/keys.c`、`vendor/ime.c` | 搬自 `rp2040-retro-dict` |
| `vendor/drivers/fatfs` | FatFs (ChaN)，BSD 類授權，見其 `ff.h` |
| `vendor/drivers/sdcard` | 搬自 `rp2040-ili9341-infones`，見其 `LICENSE` |

> ⚠️ **注音碼表的授權未定**：`vendor/ime_tables.h` 產生自
> `pico_keyboard_ime_terminal`，該 repo 目前沒有 LICENSE 檔、碼表出處也未
> 寫明。**散布前要先補上。**
