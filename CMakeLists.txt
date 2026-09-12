# rp2040-retro-editor - 掌機上的注音文件編輯器
#
# 這個專案同時是 rp2040-retro-loader 的 app 範本：五條「必修」全部做齊，
# 而且刻意不做選修的第 6 條(flash 資料區位移) —— 文件存在 SD 卡上，
# 不佔 flash，所以不需要。要抄的話，抄這一份。
#
#   一般編譯 (USB BOOTSEL 直燒)：
#       mkdir build && cd build && cmake .. && ninja
#
#   偏移編譯 (給載入器用，放 SD 卡)：
#       mkdir build_offset && cd build_offset
#       cmake .. -DLOADER_OFFSET_BUILD=ON && ninja
#
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(retro_editor C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(retro_editor
    src/main.c
    src/hw_display.c
    src/hw_keys.c
    src/doc_sd.c
    core/textbuf.c
    core/editor.c
    core/glyph.c
    vendor/lcd.c
    vendor/keys.c
    vendor/ime.c
)

target_include_directories(retro_editor PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/core
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/vendor
)

target_link_libraries(retro_editor
    pico_stdlib
    hardware_spi
    hardware_dma
)

target_compile_options(retro_editor PRIVATE -Wall -Wextra)

pico_add_extra_outputs(retro_editor)

# ---------------------------------------------------------------------------
# 必修 1 + 2 + 3：載入器支援 (預設關閉)
#
# 1. link 到 0x10004000，用載入器的 memmap_app.ld
# 2. 那份 ld 同時負責丟掉 .boot2、讓向量表落在最前面 —— 跳板跳過來之前會
#    驗向量表(SP 落在 SRAM、進入點落在 APP 區且 Thumb bit 有設)，不符合
#    就改叫 reset_usb_boot()，畫面上看起來就是「變成 USB 磁碟」
# 3. 產出兩個 uf2
#
# 為什麼不能只把檔案往後搬：RP2040 是 XIP，資料位址編譯期就寫死在機器碼
# 裡了，搬 bytes 只會讓每個指標指向錯的地方。只能重新 link。
# ---------------------------------------------------------------------------
option(LOADER_OFFSET_BUILD "Link at 0x10004000 for use with rp2040-retro-loader" OFF)

if(LOADER_OFFSET_BUILD)
    set(LOADER_PATH "${CMAKE_CURRENT_LIST_DIR}/../rp2040-retro-loader"
        CACHE PATH "rp2040-retro-loader 的路徑")

    set(LOADER_APP_LD "${LOADER_PATH}/app/memmap_app.ld")
    if(NOT EXISTS "${LOADER_APP_LD}")
        message(FATAL_ERROR
            "找不到 ${LOADER_APP_LD}\n"
            "請用 -DLOADER_PATH=<rp2040-retro-loader 的路徑> 指定。")
    endif()

    pico_set_linker_script(retro_editor "${LOADER_APP_LD}")
    message("LOADER_OFFSET_BUILD : ON  (link 到 0x10004000)")

    set(LOADER_TRAMPOLINE "${LOADER_PATH}/build/trampoline.uf2"
        CACHE FILEPATH "跳板的 uf2")

    if(EXISTS "${LOADER_TRAMPOLINE}")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        add_custom_command(TARGET retro_editor POST_BUILD
            COMMAND ${Python3_EXECUTABLE}
                    "${LOADER_PATH}/tools/merge_uf2.py"
                    "${LOADER_TRAMPOLINE}"
                    $<TARGET_FILE_DIR:retro_editor>/retro_editor.uf2
                    -o $<TARGET_FILE_DIR:retro_editor>/retro_editor_standalone.uf2
            COMMENT "合併跳板與本體 -> retro_editor_standalone.uf2"
            VERBATIM
        )
    else()
        message(WARNING
            "找不到跳板 ${LOADER_TRAMPOLINE}，略過合併步驟。\n"
            "retro_editor.uf2 仍然可以放進 SD 卡給載入器用，但不能單獨燒錄。\n"
            "要產生 standalone 版，請先在 rp2040-retro-loader 執行一次 build。")
    endif()
else()
    message("LOADER_OFFSET_BUILD : OFF (link 到 0x10000000，可直接 USB 燒錄)")
endif()
