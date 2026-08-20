# rGBA 專案程式碼審查與重構建議報告

這是一份針對 `rGBA` 專案（以 C 語言開發的 GBA 模擬器）的評估報告。本報告以「開源專案的 C 語言模擬器開發」標準為出發點，提出當前實作的優點以及未來可以優化與重構的方向。

> [!NOTE]
> 開發一款 GBA 模擬器（特別是實作 ARM7TDMI 核心與 PPU）需要對計算機架構有極深的理解。目前專案能夠成功編譯並執行，代表核心邏輯已經具備基礎，這是非常了不起的成就！

以下是針對專案各個層面的詳細評估與建議：

---

## 1. 專案架構與建置系統 (Project Structure & Build System)

目前專案的結構較為扁平，且建置環境高度依賴特定主機配置，這會增加開源社群參與貢獻的門檻。

### 觀察到的問題
*   **目錄結構過於扁平**：所有的 `.c` 和 `.h` 檔案都散落在根目錄，隨著專案增長，維護會變得困難。
*   **Makefile 寫死絕對路徑**：`Makefile` 中寫了 `LDLIBS = C:\msys64\mingw32\i686-w64-mingw32\lib`。這意味著其他開發者（Linux, macOS, 或不同安裝路徑的 Windows 使用者）無法直接編譯成功。

### 建議重構方向
> [!TIP]
> 引入現代化的建置工具與標準的目錄結構。
*   **重整目錄結構**：將程式碼分類，例如建立 `src/`（核心邏輯）、`include/`（標頭檔）以及 `frontend/`（放置 SDL 相關的 `main.c`）。
*   **改善 Makefile 或引入 CMake**：不要硬編碼（Hardcode）函式庫路徑。應使用系統標準的環境變數，或搭配 `pkg-config sdl2 --cflags --libs` 自動獲取 SDL2 路徑。如果預期會跨平台，推薦導入 `CMakeLists.txt`。

---

## 2. 記憶體管理與存取安全 (Memory Subsystem)

這是目前實作中效能瓶頸最明顯，也最容易在跨平台執行時發生崩潰的地方。

### 觀察到的問題
*   **ROM 空間嚴重浪費**：在 `memory.c` 中，分別針對 `Game_ROM1`, `Game_ROM2`, `Game_ROM3` 分配了 32MB 的記憶體，並在 `main.c` 中使用 `memcpy` 將 ROM 複製了三份。在真實硬體中，這三個區段只是具有不同存取速度（Waitstates）的**同一個實體 ROM**。
*   **指標直接轉型與解指標 (Pointer Casting)**：`io.c` 中的讀取函數（如 `MemRead32`）會將計算出的記憶體位址轉換為本機指標並直接回傳（例如 `*((uint32_t *)RelocAddr)`）。這會帶來兩個嚴重問題：
    *   **對齊問題 (Alignment)**：若 GBA 程式觸發 Unaligned Access，x86 主機可能不會崩潰，但在 ARM 等架構上會直接觸發 Segmentation Fault。GBA 硬體對未對齊讀取有特定的旋轉 (Rotate) 行為，目前的做法無法模擬。
    *   **端序問題 (Endianness)**：直接解指標假設了執行模擬器的 Host 主機與 GBA 一樣是 Little-Endian。這降低了程式碼的移植性。

### 建議重構方向
> [!WARNING]
> 直接解指標在模擬器開發中是常見的 Anti-pattern，強烈建議優先修改。
*   **共用 ROM 記憶體**：只需載入一份 ROM。當存取 `0x08000000`, `0x0A000000`, `0x0C000000` 時，在位址映射（Address Mapping）階段將它們導向同一個指標即可，這能立即省下近 70MB 的記憶體。
*   **重寫 Memory 讀寫函數**：不要直接回傳 host 指標。應該透過 bitwise 運算（`<<`, `|`）從位元組陣列中組合出 32/16-bit 數值，並手動實作 GBA 特有的 Unaligned Access 行為。

---

## 3. 架構設計與封裝 (Architecture & Encapsulation)

目前的設計將核心邏輯與特定狀態綁定得太深。

### 觀察到的問題
*   **過度依賴全域變數**：專案中大量使用了 `extern Gba_Cpu *cpu;` 和 `extern GbaMem *Mem;`。
    *   這意味著你的模擬器**永遠只能單開**。如果未來想要實作 GBA 通訊線 (Link Cable) 等需要多實例運行的功能，會遇到極大的阻礙。
*   **核心與前端耦合**：在 `main.c` 裡，CPU 的執行迴圈與 SDL2 的視窗繪製、事件處理混雜在一起。如果未來想將核心移植到 Libretro (RetroArch) 或 WebAssembly，抽離核心代碼會很困難。

### 建議重構方向
> [!IMPORTANT]
> 良好的封裝是專案能長期發展的基石。
*   **實作 Context Struct**：將所有的模擬器狀態（CPU, 記憶體, 暫存器）打包成一個大型的結構體（例如 `struct GBA_Context`），並將這個結構體的指標傳遞給每個處理函數，徹底消滅全域變數。
*   **核心與前端解耦**：將所有的 SDL2 邏輯移出核心迴圈，核心只需提供 `GBA_RunFrame()` 等 API 供前端呼叫，繪圖時提供 framebuffer 指標即可。

---

## 4. 週期同步與硬體 I/O (Cycle Accuracy & Hardware Sync)

### 觀察到的問題
*   **巨觀同步 (Macro-timing)**：在 `main.c` 透過累加 `cpu->frame_cycle >= 1232` 來觸發 `PPU_update`。這屬於較粗略的同步方式。對時序極度敏感的遊戲（例如依賴 HBlank 進行光柵特效的遊戲）可能會出現破圖或當機。
*   **I/O 處理分散**：目前 IO 的 Side-effects 直接寫在 `MemWrite` 內部。

### 建議重構方向
*   **引入事件系統或更細粒度的同步**：考慮實作一個基於週期的排程系統 (Scheduler)，讓 PPU, Timer, DMA 能夠在正確的 Clock 週期被精確觸發。
*   **獨立 IO Handler**：建立統一的 IO 處理模組，攔截對 `0x04000000` 區段的讀寫，統一管理硬體狀態改變。

---

## 總結

這份專案作為個人的學習里程碑，表現十分優異。下一步的重點建議**不要急著增加新功能，而是先進行重構 (Refactoring)**。可以從最急迫的**消除 Makefile 環境依賴**與**修復記憶體 ROM 的重複配置**開始著手，接著逐步消除全域變數，最終將專案打造成為一個架構乾淨、跨平台的開源 GBA 核心。
