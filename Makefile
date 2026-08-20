CC = clang
CFLAGS = -I./include -Wall -Wextra -g
BIN_DIR = bin
SRC_DIR = src
TEST_DIR = tests

# 主程式原始碼 (全部的 .c 檔)
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
# 測試用的共用原始碼 (排除 main.c，避免與測試檔的 main 衝突)
CORE_SRC_FILES = $(filter-out $(SRC_DIR)/main.c, $(SRC_FILES))

# 執行檔名稱
MAIN_TARGET = $(BIN_DIR)/rGBA.exe
TEST_TARGET = $(BIN_DIR)/test_io.exe

# 定義為偽目標 (不會產生同名檔案)
.PHONY: all clean test

# 預設行為：編譯主程式
all: $(BIN_DIR) $(MAIN_TARGET)

# 建立 bin 目錄
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 編譯主程式
$(MAIN_TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# ================================
# 測試專用區塊
# ================================

# 當執行 make test 時，會先確保編譯出 TEST_TARGET，然後執行它
test: $(BIN_DIR) $(TEST_TARGET)
	@echo "Running Tests..."
	./$(TEST_TARGET)

# 編譯測試程式
$(TEST_TARGET): $(TEST_DIR)/test_io.c $(CORE_SRC_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# 清除編譯產生的檔案
clean:
	rm -rf $(BIN_DIR)/*.exe
