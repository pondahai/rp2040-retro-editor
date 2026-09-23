# Third-Party Notices

本 repo 的 `vendor/` 目錄收錄了第三方程式碼與衍生資料。
散布本專案時,以下聲明必須隨著一起散布。

專案自身的程式碼(`core/`、`src/`)採 MIT,見 `LICENSE`。

---

## 1. 注音碼表 — McBopomofo(小麥注音輸入法)

`vendor/ime_tables.h` 是注音碼表的衍生資料,經由
[ime-charset-font-bitmap](https://github.com/pondahai/ime-charset-font-bitmap)
的管線,從 McBopomofo 的碼表轉換而成。

| | |
|---|---|
| 上游 | https://github.com/openvanilla/McBopomofo |
| 授權 | MIT License |
| 著作權 | Copyright (c) 2011-2026 Mengjuei Hsieh et al. |

使用的是 `BPMFBase.txt`(單字注音)與 `BPMFPunctuations.txt`(標點),
兩者在 McBopomofo 內皆無額外上游出處,適用該專案的 MIT。

**未使用** `BPMFMappings.txt` —— 依 McBopomofo 的 `Source/Data/README.md`,
只有該多字詞庫檔帶有 libtabe(BSD)的血統。本專案是單字候選而非詞庫,
故不涉及 libtabe。

### MIT License 全文

```
MIT License

Copyright (c) 2011-2026 Mengjuei Hsieh et al.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 2. 中文字型 — Cubic 11(俐方體十一號)

`vendor/font_cjk.h` 是 Cubic 11 光柵化後的 1bpp 點陣資料。

| | |
|---|---|
| 上游 | https://github.com/ACh-K/Cubic-11 |
| 授權 | SIL Open Font License 1.1 |

> **保留字型名稱(Reserved Font Name)**:Cubic 保留「Cubic」「俐方體」。
> 本專案的衍生資料不得以這些名稱對外呈現為字型名。

依 SIL OFL 1.1,散布衍生資料時須隨附授權文字。

---

## 3. FatFs

`vendor/drivers/fatfs/` —— FatFs Generic FAT Filesystem Module R0.14b。

| | |
|---|---|
| 上游 | http://elm-chan.org/fsw/ff/ |
| 授權 | BSD 類(1-clause),全文見 `vendor/drivers/fatfs/ff.h` 檔頭 |
| 著作權 | Copyright (C) 2021, ChaN, all right reserved. |

---

## 4. SD 卡驅動

`vendor/drivers/sdcard/` —— PIO SPI SD 卡驅動。

| 檔案 | 授權 | 著作權 |
|---|---|---|
| `sdcard.c/h`、`spi.pio` 等 | BSD-2-Clause,全文見 `vendor/drivers/sdcard/LICENSE` | Copyright (c) 2020-2021, Elehobica |
| `pio_spi.c/h` | BSD-3-Clause(SPDX 標於檔頭) | Copyright (c) 2020 Raspberry Pi (Trading) Ltd. |

---

## 5. 同生態系專案

下列檔案搬自作者自己的其他 repo,著作權同屬本專案作者,
於本專案中以 MIT 散布:

| 檔案 | 來源 |
|---|---|
| `vendor/lcd.c` | [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader) |
| `vendor/keys.c`、`vendor/ime.c` | [rp2040-retro-dict](https://github.com/pondahai/rp2040-retro-dict) |

> `rp2040-retro-dict` 整體以 GPL-3.0 散布(因其收錄了 GPL-3.0 的
> `audio.c` 與 `TFT_DMA.*`)。`keys.c` 與 `ime.c` 為本專案作者原創、
> 不含那些 GPL 成分,著作權人得另以 MIT 授權於此處散布。
