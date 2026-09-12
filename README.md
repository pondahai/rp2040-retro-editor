# rp2040-retro-editor

RP2040 掌機上的**注音中文文件編輯器**，文件存在 SD 卡上。

同時是 [`rp2040-retro-loader`](https://github.com/pondahai/rp2040-retro-loader)
的 **app 範本** —— 要寫新的掌機韌體，可以從這份抄。

> 🚧 **開發中**。LCD、鍵盤矩陣、注音組字、編輯核心、中文字型都通了，
> **SD 卡存取還沒接上**（見下方「還沒做」）。

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
src/      只有這一層碰 GPIO
          hw_keys.c   74HC595/165 掃描時序
          hw_display.c  版面
          doc_sd.c    SD 存取（目前是 stub）
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

> 💡 若 cmake 抱怨找不到 picotool，加
> `-Dpicotool_DIR=<你的>/.pico-sdk/picotool/<版本>/picotool`。

---

## 操作

| 鍵 | 動作 |
| :--- | :--- |
| `Fn` + `Space` | 切換英數 / 注音 |
| `Fn` + `S` | 存檔 |
| 注音模式：注音鍵 | 組字（大千配列，一聲不用打） |
| 注音模式：`1`–`9` | 選字 |
| 注音模式：`Space` | 候選字翻頁 |
| 注音模式：`Esc` | 取消組字 |

輸入走 **8×8 鍵盤矩陣**（64 鍵實體 QWERTY），不是 D-pad。
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
| **SD 卡存取** | `src/doc_sd.c` 是 stub，每個函式回 `DOC_ENOIMPL`。刻意不做「假裝成功」的版本 —— 那會讓使用者以為存檔了 |
| ~~中文字型~~ | ✅ 已接上 Cubic 11，見下節 |
| **真機測試** | 尚未在板子上跑過。編譯與向量表驗證過了，畫面沒有 |

### SD 寫入是這個專案真正的工程量

生態系現有的韌體（infones / PicoApple2 / doom / loader）**全部只讀 SD**。
編輯器是第一個要寫的，所以掉電損毀、`f_sync` 的時機、`FF_FS_READONLY=0`
之後多出來的 code size 都是新題目。FatFs 與 SD 驅動可以從
`rp2040-ili9341-infones/software/infones/drivers/` 搬，但那是唯讀用法。

---

## 授權與出處

| 來源 | 授權 |
| :--- | :--- |
| `vendor/font_cjk.h` | Cubic 11（俐方體十一號），OFL；經 infones 的 `make_cjk_font.py` 重新打包 |
| `vendor/lcd.c` | 搬自 `rp2040-retro-loader` |
| `vendor/keys.c`、`vendor/ime.c` | 搬自 `rp2040-retro-dict` |

> ⚠️ **注音碼表的授權未定**：`vendor/ime_tables.h` 產生自
> `pico_keyboard_ime_terminal`，該 repo 目前沒有 LICENSE 檔、碼表出處也未
> 寫明。**散布前要先補上。**
