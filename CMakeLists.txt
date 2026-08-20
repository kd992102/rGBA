cmake_minimum_required(VERSION 3.15)
project(rGBA C)

# 設定 C 語言標準 (GBA 開發通常用 C99 即可)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# 設定編譯警告
add_compile_options(-Wall -Wextra -g)

# 加入 include 目錄，這樣所有檔案都找得到 <gba.h> 和 <io.h>
include_directories(include)

# 找出所有的 src/*.c 檔案
file(GLOB ALL_SRC "src/*.c")

# 從原始碼清單中移除 main.c，將剩下的當作「共用核心程式」
list(REMOVE_ITEM ALL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
set(CORE_SRC ${ALL_SRC})

# 1. 編譯主程式 (包含 main.c + 其他核心)
add_executable(rGBA src/main.c ${CORE_SRC})

# ================================
# 測試專區
# ================================
# 啟用 CTest 測試功能
enable_testing()

# 2. 編譯 I/O 測試程式 (包含 test_io.c + 其他核心)
add_executable(test_io tests/test_io.c ${CORE_SRC})

# 註冊一個叫做 IOTest 的測試，執行 test_io
add_test(NAME IOTest COMMAND test_io)
