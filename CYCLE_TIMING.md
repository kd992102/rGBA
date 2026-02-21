# ARM7TDMI Cycle Timing 參考

> **文件更新**: 已根據本地 gbatek.html 檔案驗證和增強 (2024)
> 
> **關鍵發現**:
> - ✅ WAITCNT 預設啟動值為 `0000h` (不是之前假設的值)
> - ✅ 實際訪問時間 = **1 clock cycle + waitstate 值**
> - ✅ WS1/WS2 的 Second Access 值與 WS0 不同 (4/1 cycles 和 8/1 cycles)
> - ✅ 32-bit 訪問到 16-bit 匯流排自動分成兩個 16-bit (第2個總是 S-cycle)
> - ✅ GBA 在 128K 邊界強制使用 N-cycle
> - ✅ Prefetch buffer 容量: 8 個 16-bit 值
> - ✅ 一個週期 ≈ 59.59ns (16.78MHz)

## Cycle 型別 (根據 arm7tdmi.txt 6.2節 + GBATEK)

| nMREQ | SEQ | 型別 | 說明 (ARM7TDMI) | GBATEK 詳細說明 |
|-------|-----|------|----------------|----------------|
| 0 | 0 | **N-cycle** | 訪問與前一個地址無關的新地址 | 請求傳輸到與前一週期**無關**的地址 (Called "1st Access" in GBA) |
| 0 | 1 | **S-cycle** | 訪問連續地址（相同或+word/halfword） | 請求傳輸到**緊接著**前一週期地址的位置 (Called "2nd Access" in GBA) |
| 1 | 0 | **I-cycle** | CPU內部操作，不需要記憶體訪問 | CPU 忙碌中，暫時不請求記憶體傳輸 |
| 1 | 1 | **C-cycle** | 協處理器暫存器傳輸 | CPU 使用資料匯流排與協處理器通訊，但不請求記憶體傳輸 |

**重要**:
- **N-cycle 執行時間** = 1 clock cycle + non-sequential access waitstates
- **S-cycle 執行時間** = 1 clock cycle + sequential access waitstates  
- **I-cycle 執行時間** = 1 clock cycle (無 waitstates)
- **C-cycle** = 協處理器通訊 (GBA 無協處理器)

## 指令 Cycle Timing (ARM 模式)

### Branch/BL (分支)
- **Cycle Count**: `2S + 1N`
- **說明**: 
  - Cycle 1: 計算分支目標，從當前PC預取 (S)
  - Cycle 2: 從分支目標fetch，可能更新R14 (N)
  - Cycle 3: 從目標+4 fetch，重新填充pipeline (S)

### BX (分支交換)
- **Cycle Count**: `2S + 1N`
- **說明**: 
  - Cycle 1: 提取分支目標和新狀態 (S)
  - Cycle 2: 從新狀態的分支目標fetch (N)
  - Cycle 3: 重新填充pipeline (S)

### Data Processing (資料處理)
- **Cycle Count**: `1S` (立即數或暫存器移位)
- **Cycle Count**: `1S + 1I` (暫存器控制的移位)
- **說明**:
  - 基本操作：1個S-cycle
  - 暫存器移位：額外1個I-cycle獲取移位量
  - 寫入PC：額外 `+2S + 1N` (變成分支)

### Multiply (乘法)
- **MUL**: `1S + mI`
- **MLA**: `1S + (m+1)I`  
- **m**: 根據運算元的值（1-4個cycle）

### Long Multiply (長乘法)
- **MULL**: `1S + (m+1)I`
- **MLAL**: `1S + (m+2)I`

### LDR (載入暫存器)
- **Cycle Count**: `1S + 1N + 1I`
- **說明**:
  - Cycle 1: 計算地址 (S)
  - Cycle 2: 從記憶體讀取資料 (N)
  - Cycle 3: 資料傳輸到暫存器 (I)
- **載入到PC**: 額外 `+2S + 1N`

### STR (儲存暫存器)
- **Cycle Count**: `2N`
- **說明**:
  - Cycle 1: 計算地址 (N)
  - Cycle 2: 寫入資料到記憶體 (N)

### LDM (載入多個暫存器)
- **Cycle Count**: `nS + 1N + 1I`
  - n = 暫存器數量
  - 第一個載入是N-cycle，後續是S-cycle
- **載入PC**: 額外 `+2S + 1N`

### STM (儲存多個暫存器)
- **Cycle Count**: `(n-1)S + 2N`
  - n = 暫存器數量
  - 第一個和最後一個儲存是N-cycle

### SWP (資料交換)
- **Cycle Count**: `1S + 2N + 1I`

### SWI/Exception
- **Cycle Count**: `2S + 1N`

## 指令 Cycle Timing (THUMB 模式)

### Branch (條件分支)
- **Cycle Count**: `2S + 1N`
- **說明**: 類似ARM分支，但指令寬度為2位元組

### Branch (無條件分支)
- **Cycle Count**: `2S + 1N`

### BL (長分支連結) - 兩條指令
- **第一條指令**: `1S` (計算高位偏移)
- **第二條指令**: `2S + 1N` (完成分支)
- **總計**: `3S + 1N`

### BX (分支交換)
- **Cycle Count**: `2S + 1N`

### Data Processing
- **Cycle Count**: `1S`
- **說明**: 大部分THUMB資料處理指令

### LDR/STR
- **LDR**: `1S + 1N + 1I`
- **STR**: `2N`

### LDM/STM
- **LDM**: `nS + 1N + 1I` (n=暫存器數量)
- **STM**: `(n-1)S + 2N` (n=暫存器數量)

### PUSH/POP
- **PUSH**: 等同於STM
- **POP**: 等同於LDM
- **POP {PC}**: 額外 `+2S + 1N`

## GBA 記憶體等待週期 (基於 GBATEK)

### 記憶體訪問時序表

**完整的 GBA 記憶體訪問週期表 (來源: gbatek.html 官方文件)**

| 區域 | 地址範圍 | 匯流排寬度 | 讀支援 | 寫支援 | Cycles (8/16/32-bit) | 說明 |
|------|----------|----------|--------|--------|---------------------|------|
| **BIOS ROM** | 0x0000000-0x0003FFF | 32-bit | 8/16/32 | - | 1/1/1 | 系統 ROM, 只讀, 最快 |
| **IWRAM** | 0x3000000-0x3007FFF | 32-bit | 8/16/32 | 8/16/32 | 1/1/1 | 內部 32KB, 最快 |
| **I/O** | 0x4000000-0x40003FF | 32-bit | 8/16/32 | 8/16/32 | 1/1/1 | I/O 暫存器 |
| **OAM** | 0x7000000-0x70003FF | 32-bit | 8/16/32 | 16/32 | 1/1/1 * | OBJ屬性, 無8位寫 |
| **EWRAM** | 0x2000000-0x203FFFF | 16-bit | 8/16/32 | 8/16/32 | 3/3/6 ** | 外部 256KB, 2 wait |
| **PAL RAM** | 0x5000000-0x50003FF | 16-bit | 8/16/32 | 16/32 | 1/1/2 * | 調色盤 1KB, 無8位寫 |
| **VRAM** | 0x6000000-0x6017FFF | 16-bit | 8/16/32 | 16/32 | 1/1/2 * | 影片 96KB, 無8位寫 |
| **ROM WS0** | 0x8000000-0x9FFFFFF | 16-bit | 8/16/32 | - | 5/5/8 **/** | 等待狀態 0 (預設) |
| **ROM WS1** | 0xA000000-0xBFFFFFF | 16-bit | 8/16/32 | - | 5/5/8 **/** | 等待狀態 1 |
| **ROM WS2** | 0xC000000-0xDFFFFFF | 16-bit | 8/16/32 | - | 5/5/8 **/** | 等待狀態 2 |
| **Flash ROM** | 0x8000000-0xDFFFFFF | 16-bit | 8/16/32 | 16/32 | 5/5/8 **/** | 可寫 Flash |
| **SRAM** | 0xE000000-0xE00FFFF | 8-bit | 8 | 8 | 5 ** | 僅8位訪問 |

**Timing Notes (直接來自 GBATEK):**
- `*` Plus 1 cycle if GBA accesses video memory at the same time (CPU-PPU conflict)
- `**` Default waitstate settings, configurable via WAITCNT register (0x4000204)
- `***` Separate timings for sequential and non-sequential accesses
- **One cycle equals approximately 59.59ns (16.78MHz clock)**
- **32-bit accesses on 16-bit bus**: Split into TWO 16-bit accesses (2nd is always sequential, even if 1st was non-sequential)

### 等待狀態控制暫存器 (0x4000204 - WAITCNT) [來源: gbatek.html]

ROM 和 SRAM 的等待週期可以透過 WAITCNT 暫存器配置：

```
Bit   Name            Default  說明 (根據 GBATEK)
0-1   SRAM Wait       0        SRAM 訪問時間 (0..3 = 4,3,2,8 cycles)
2-3   WS0 First       0        ROM WS0 首次訪問 (0..3 = 4,3,2,8 cycles)  
4     WS0 Second      0        ROM WS0 連續訪問 (0..1 = 2,1 cycles)
5-6   WS1 First       0        ROM WS1 首次訪問 (0..3 = 4,3,2,8 cycles)
7     WS1 Second      0        ROM WS1 連續訪問 (0..1 = 4,1 cycles; unlike WS0)
8-9   WS2 First       0        ROM WS2 首次訪問 (0..3 = 4,3,2,8 cycles)
10    WS2 Second      0        ROM WS2 連續訪問 (0..1 = 8,1 cycles; unlike WS0,WS1)
11-12 PHI Terminal    0        PHI 終端輸出 (0..3 = Disable, 4.19MHz, 8.38MHz, 16.78MHz)
13    -               0        未使用
14    Prefetch        0        GamePak 預取緩衝區 (0=禁用, 1=啟用)
15    Game Pak Type   0        只讀: 遊戲卡型別 (0=GBA, 1=CGB) - IN35 訊號
```

**重要**:
- 啟動時預設值: `0000h` (所有 waitstate 為預設最慢值)
- 商業卡帶常用: `4317h` (WS0/ROM=3,1 clks; SRAM=8 clks; Prefetch enabled)
- **實際訪問時間 = 1 clock cycle + waitstate 值** (GBATEK 明確說明)
- GamePak 使用 16bit 資料匯流排 → 32bit 訪問分成兩個 16bit (第二個總是 sequential，即使第一個是 non-sequential)

### 重要說明

1. **32位訪問 = 兩次16位訪問**
   - 32位訪問到16位匯流排會分成兩次16位訪問
   - 第一次是 N-cycle，第二次是 S-cycle
   - 例如：EWRAM 32位讀取 = 3 (N) + 3 (S) = 6 cycles

2. **BIOS 訪問限制**
   - BIOS 區域只能在 PC 位於 BIOS 內時訪問
   - 從外部訪問會返回最後一次 BIOS fetch 的值

3. **Video Memory 訪問衝突**
   - 當 PPU 訪問 VRAM/OAM/PAL 時，CPU 訪問會被延遲
   - 在 H-Blank 和 V-Blank 期間訪問最快

4. **GamePak Prefetch Buffer** (GBATEK詳細說明)
   - 啟用後可以在 CPU 執行指令時預取下一條 ROM 指令
   - 對於順序執行的程式碼可以大幅提升效能
   - 分支會清空預取緩衝區
   - **Buffer容量**: 最多儲存 8 個 16bit 值
   - **僅對GamePak ROM opcodes有效** - 在 RAM/BIOS 執行程式碼時不起作用
   - **Prefetch Enable 條件**:
     1. 有內部週期 (I) 且不改變 R15 的指令 (暫存器移位、load、multiply)
     2. Load/Store 記憶體的指令 (ldr,str,ldm,stm等)
   - **Prefetch Disable Bug**: 當禁用時，包含 I-cycle 且不改變 R15 的指令會從 1S 變成 1N

5. **128K 邊界強制 Non-Sequential** (GBATEK重要說明)
   - GBA 強制在每個 128K 塊的開始使用 non-sequential timing
   - 例如: `LDMIA [0x801FFF8],r0-r7` 會在 0x8020000 處使用 N-cycle timing
   - 這對最佳化記憶體訪問很重要！

### 實際 Cycle 計算示例 (基於 GBATEK 準確資料)

```c
// 示例 1: BIOS 中的順序 ARM 指令
// ADD r0, r1, r2  (1S cycle from CPU)
// BIOS ROM: 32-bit bus, 1/1/1 cycles
// 實際週期: 1 cycle (S-cycle + 0 waitstates)

// 示例 2: IWRAM 中的 32位資料載入
// LDR r0, [r1]  (1S + 1N + 1I from CPU)
// IWRAM: 32-bit bus, 1/1/1 cycles
// 實際週期: 1 (S) + 1 (N) + 1 (I) = 3 cycles

// 示例 3: EWRAM 中的 32位資料載入
// LDR r0, [r1]  (1S + 1N + 1I from CPU)
// EWRAM: 16-bit bus, 3/3/6 cycles for 8/16/32-bit
// - S-cycle (opcode fetch): 1 + 0 = 1 cycle (假設從快速記憶體執行)
// - N-cycle (32-bit load): 分成兩個 16-bit: 3 (N) + 3 (S) = 6 cycles
// - I-cycle (data transfer): 1 cycle
// 實際週期: 1 + 6 + 1 = 8 cycles

// 示例 4: ROM 中的分支 (WS0, WAITCNT=0000h 預設)
// B label  (2S + 1N from CPU)
// ROM WS0 預設: First=4, Second=2 → 實際 = 1+4=5 (N), 1+2=3 (S)
// - 2× S-cycle (opcode fetch): 2×3 = 6 cycles
// - 1× N-cycle (branch target): 1×5 = 5 cycles
// 實際週期: 6 + 5 = 11 cycles

// 示例 5: ROM 中的分支 (WS0, WAITCNT=4317h 常用值)
// WAITCNT=4317h → WS0: First=2→1+3=4, Second=1→1+1=2
// B label  (2S + 1N from CPU)
// - 2× S-cycle: 2×2 = 4 cycles
// - 1× N-cycle: 1×4 = 4 cycles
// 實際週期: 4 + 4 = 8 cycles

// 示例 6: ROM 中的 32-bit load (WS0, WAITCNT=4317h, Prefetch enabled)
// LDR r0, [r1]位於IWRAM, 從ROM地址讀取  (1S + 1N + 1I from CPU)
// ROM: 16-bit bus, 32-bit 訪問需要兩次 16-bit
// - S-cycle (opcode fetch from IWRAM): 1 cycle
// - N-cycle (32-bit from ROM): 首次 N (1+3=4), 然後 S (1+1=2) = 6 cycles
// - I-cycle (data transfer): 1 cycle
// 實際週期: 1 + 6 + 1 = 8 cycles
// (如果 prefetch 命中, S-cycle 可能為 0)

// 示例 7: 128K 邊界跨越
// LDMIA [0x801FFF8], {r0-r7}  
// 在 0x8020000 處, GBA 強制使用 N-cycle (非 S-cycle)
// 這會導致額外的 waitstate penalty
```

## 實現建議

### 從 GBATEK 學到的關鍵實現要點

#### 1. Waitstate 計算公式 (CRITICAL)
```c
// GBATEK 明確說明: "the actual access time is 1 clock cycle PLUS the waitstates"
actual_cycles = 1 + waitstate_value;

// 例如: WAITCNT WS0 First Access = 3
// 實際 N-cycle = 1 + 3 = 4 cycles
```

#### 2. 32-bit 訪問到 16-bit 匯流排
```c
// GBATEK: "GamePak uses 16bit data bus, so that a 32bit access is split 
//          into TWO 16bit accesses (of which, the second fragment is 
//          always sequential, even if the first fragment was non-sequential)"

if (bus_width == 16 && access_size == 32) {
    first_cycle = is_sequential ? S_cycle : N_cycle;
    second_cycle = S_cycle;  // 總是 sequential!
    total_cycles = first_cycle + second_cycle;
}
```

#### 3. 128K 邊界強制 Non-Sequential
```c
// GBATEK: "The GBA forcefully uses non-sequential timing at the beginning 
//          of each 128K-block of gamepak ROM"

if ((address & 0x1FFFF) == 0 && address >= 0x08000000 && address <= 0x0DFFFFFF) {
    // 強制使用 N-cycle，即使正常應該是 S-cycle
    force_nonsequential = true;
}
```

#### 4. WAITCNT 位欄位解碼
```c
// GBATEK 準確的位定義
typedef struct {
    uint8_t sram_wait;      // 0-1: 0..3 = 4,3,2,8 cycles
    uint8_t ws0_first;      // 2-3: 0..3 = 4,3,2,8 cycles
    uint8_t ws0_second;     // 4:   0..1 = 2,1 cycles
    uint8_t ws1_first;      // 5-6: 0..3 = 4,3,2,8 cycles
    uint8_t ws1_second;     // 7:   0..1 = 4,1 cycles (注意: 與WS0不同!)
    uint8_t ws2_first;      // 8-9: 0..3 = 4,3,2,8 cycles
    uint8_t ws2_second;     // 10:  0..1 = 8,1 cycles (注意: 與WS0,WS1不同!)
    uint8_t phi_output;     // 11-12: PHI terminal output
    uint8_t prefetch_enable;// 14:  Prefetch buffer enable
    uint8_t gamepak_type;   // 15:  只讀，0=GBA, 1=CGB
} WAITCNT_Decoded;

// 解碼函式
const uint8_t sramws_table[4] = {4, 3, 2, 8};
const uint8_t ws_first_table[4] = {4, 3, 2, 8};
const uint8_t ws0_second_table[2] = {2, 1};
const uint8_t ws1_second_table[2] = {4, 1};
const uint8_t ws2_second_table[2] = {8, 1};
```

#### 5. Prefetch Buffer 實現考慮
```c
// GBATEK: "The prefetch buffer stores up to eight 16bit values"
#define PREFETCH_BUFFER_SIZE 8  // 8 個 16-bit 值

typedef struct {
    uint16_t buffer[PREFETCH_BUFFER_SIZE];
    uint8_t count;      // 當前緩衝的指令數
    uint32_t next_addr; // 下一個預取地址
    bool enabled;       // WAITCNT bit 14
} PrefetchBuffer;

// GBATEK: "prefetch may occur in two situations:"
// 1. Opcodes with internal cycles (I) which do not change R15
// 2. Opcodes that load/store memory
bool can_prefetch = (has_internal_cycle && !changes_pc) || is_load_store;
```

### 基礎 Cycle 型別和結構

```c
typedef enum {
    CYCLE_N = 0,  // Non-sequential
    CYCLE_S = 1,  // Sequential  
    CYCLE_I = 2,  // Internal
    CYCLE_C = 3   // Coprocessor
} CycleType;

typedef struct {
    uint8_t cycles;          // 基礎週期數（ARM7TDMI 標準）
    CycleType types[8];      // 每個週期的型別（最多8個）
    uint8_t type_count;      // 實際週期型別數量
    bool branch_taken;
    bool pipeline_flush;
    
    // GBA 特定資訊
    uint32_t fetch_addr;     // 取指地址（用於計算等待狀態）
    uint32_t access_addr;    // 記憶體訪問地址（如果有）
    bool is_32bit;           // 是否為32位訪問
} InstructionResult;
```

### GBA Memory Timing 實現

```c
// 獲取 GBA 特定的記憶體訪問週期
uint8_t GBA_GetMemoryAccessCycles(GBA_Core *core, uint32_t address, 
                                   bool sequential, bool is_32bit) {
    uint8_t region = (address >> 24) & 0xF;
    uint8_t cycles = 1;
    
    switch (region) {
        case 0x0: // BIOS
            cycles = 1;
            break;
            
        case 0x2: // EWRAM (256KB)
            cycles = is_32bit ? 6 : 3;  // 32位 = 兩次16位訪問
            break;
            
        case 0x3: // IWRAM (32KB)
            cycles = 1;
            break;
            
        case 0x4: // I/O
            cycles = 1;
            break;
            
        case 0x5: // Palette RAM
            cycles = is_32bit ? 2 : 1;
            // TODO: 檢查 PPU 訪問衝突
            break;
            
        case 0x6: // VRAM
            cycles = is_32bit ? 2 : 1;
            // TODO: 檢查 PPU 訪問衝突
            break;
            
        case 0x7: // OAM
            cycles = 1;
            // TODO: 檢查 PPU 訪問衝突
            break;
            
        case 0x8: // ROM WS0
        case 0x9:
            cycles = GBA_GetROMWaitCycles(core, 0, sequential, is_32bit);
            break;
            
        case 0xA: // ROM WS1
        case 0xB:
            cycles = GBA_GetROMWaitCycles(core, 1, sequential, is_32bit);
            break;
            
        case 0xC: // ROM WS2
        case 0xD:
            cycles = GBA_GetROMWaitCycles(core, 2, sequential, is_32bit);
            break;
            
        case 0xE: // SRAM
        case 0xF:
            cycles = GBA_GetSRAMWaitCycles(core);
            break;
            
        default:
            cycles = 1;
            break;
    }
    
    return cycles;
}

// 獲取 GamePak ROM 等待週期 (根據 GBATEK 準確資料)
uint8_t GBA_GetROMWaitCycles(GBA_Core *core, uint8_t ws_number, 
                             bool sequential, bool is_32bit) {
    uint16_t waitcnt = core->memory.io_registers[0x204] | 
                      (core->memory.io_registers[0x205] << 8);
    
    // GBATEK 準確的查詢表
    const uint8_t first_access_table[4] = {4, 3, 2, 8};  // 0..3 = 4,3,2,8
    const uint8_t ws0_second_table[2] = {2, 1};          // 0..1 = 2,1
    const uint8_t ws1_second_table[2] = {4, 1};          // 0..1 = 4,1 (不同!)
    const uint8_t ws2_second_table[2] = {8, 1};          // 0..1 = 8,1 (不同!)
    
    uint8_t n_cycles, s_cycles;
    
    switch (ws_number) {
        case 0: // WS0
            n_cycles = first_access_table[(waitcnt >> 2) & 3];  // Bits 2-3
            s_cycles = ws0_second_table[(waitcnt >> 4) & 1];    // Bit 4
            break;
            
        case 1: // WS1
            n_cycles = first_access_table[(waitcnt >> 5) & 3];  // Bits 5-6
            s_cycles = ws1_second_table[(waitcnt >> 7) & 1];    // Bit 7
            break;
            
        case 2: // WS2
            n_cycles = first_access_table[(waitcnt >> 8) & 3];  // Bits 8-9
            s_cycles = ws2_second_table[(waitcnt >> 10) & 1];   // Bit 10
            break;
            
        default:
            n_cycles = 4;
            s_cycles = 2;
            break;
    }
    
    uint8_t cycles;
    
    if (is_32bit) {
        // GBATEK: "32bit access is split into TWO 16bit accesses 
        //          (of which, the second fragment is always sequential)"
        if (sequential) {
            cycles = s_cycles + s_cycles;  // 兩個都是 S
        } else {
            cycles = n_cycles + s_cycles;  // 第一個 N，第二個強制 S
        }
    } else {
        cycles = sequential ? s_cycles : n_cycles;
    }
    
    return cycles;
}

// 獲取 SRAM 等待週期 (根據 GBATEK 準確資料)
uint8_t GBA_GetSRAMWaitCycles(GBA_Core *core) {
    uint16_t waitcnt = core->memory.io_registers[0x204] | 
                      (core->memory.io_registers[0x205] << 8);
    
    // GBATEK: "SRAM Wait Control (0..3 = 4,3,2,8 cycles)"
    const uint8_t sram_wait_table[4] = {4, 3, 2, 8};
    uint8_t sram_wait = waitcnt & 3;  // Bits 0-1
    
    return sram_wait_table[sram_wait];
}
}

// 檢查是否在 Video Memory 訪問衝突期間
bool GBA_IsVideoMemoryConflict(GBA_Core *core, uint32_t address) {
    uint8_t region = (address >> 24) & 0xF;
    
    // 只有 PAL/VRAM/OAM 受影響
    if (region < 0x5 || region > 0x7) {
        return false;
    }
    
    // 在 H-Blank 或 V-Blank 期間沒有衝突
    uint16_t dispstat = core->ppu.DISPSTAT;
    bool in_hblank = (dispstat & 0x0002) != 0;
    bool in_vblank = (dispstat & 0x0001) != 0;
    
    if (in_hblank || in_vblank) {
        return false;
    }
    
    // TODO: 更精確的 PPU 訪問時序檢查
    // 目前簡化為：在可見掃描期間所有訪問都可能衝突
    return true;
}
```

### Prefetch Buffer 實現（可選，高階最佳化）

```c
typedef struct {
    bool enabled;           // 是否啟用預取
    uint32_t addr;          // 預取地址
    uint16_t buffer[8];     // 預取緩衝區（最多8個halfword）
    uint8_t count;          // 已預取數量
    uint8_t head;           // 讀取位置
} GBA_PrefetchBuffer;

// 檢查預取緩衝區是否命中
bool GBA_PrefetchHit(GBA_Core *core, uint32_t fetch_addr) {
    GBA_PrefetchBuffer *prefetch = &core->memory.prefetch;
    
    if (!prefetch->enabled) {
        return false;
    }
    
    // 檢查地址是否在預取範圍內
    if (fetch_addr >= prefetch->addr && 
        fetch_addr < prefetch->addr + (prefetch->count * 2)) {
        return true;
    }
    
    return false;
}

// 清空預取緩衝區（分支時）
void GBA_PrefetchClear(GBA_Core *core) {
    core->memory.prefetch.count = 0;
    core->memory.prefetch.head = 0;
}
```

## 計算總實際週期

```c
// 根據指令結果計算實際 GBA 週期
uint32_t GBA_CalculateActualCycles(GBA_Core *core, InstructionResult *result) {
    uint32_t total = 0;
    uint32_t current_addr = result->fetch_addr;
    bool is_arm = (core->cpu.regs.exec_mode == CPU_MODE_ARM);
    uint8_t instr_width = is_arm ? 4 : 2;
    
    for (int i = 0; i < result->type_count; i++) {
        CycleType type = result->types[i];
        uint8_t wait_cycles;
        
        switch (type) {
            case CYCLE_N: {
                // Non-sequential access
                bool is_32bit = is_arm || (result->is_32bit && i > 0);
                wait_cycles = GBA_GetMemoryAccessCycles(core, current_addr, 
                                                        false, is_32bit);
                current_addr += instr_width;
                break;
            }
            
            case CYCLE_S: {
                // Sequential access
                bool is_32bit = is_arm || (result->is_32bit && i > 0);
                
                // 檢查預取緩衝區
                if (GBA_PrefetchHit(core, current_addr)) {
                    wait_cycles = 1;  // 預取命中，立即可用
                } else {
                    wait_cycles = GBA_GetMemoryAccessCycles(core, current_addr, 
                                                            true, is_32bit);
                }
                current_addr += instr_width;
                break;
            }
            
            case CYCLE_I: {
                // Internal cycle - always 1
                wait_cycles = 1;
                break;
            }
            
            case CYCLE_C: {
                // Coprocessor cycle - always 1 (GBA 沒有協處理器)
                wait_cycles = 1;
                break;
            }
            
            default:
                wait_cycles = 1;
                break;
        }
        
        // 檢查 Video Memory 訪問衝突
        if (type != CYCLE_I && GBA_IsVideoMemoryConflict(core, current_addr)) {
            wait_cycles += 1;  // 額外延遲1-2個週期
        }
        
        total += wait_cycles;
    }
    
    return total;
}

// 在指令執行後更新週期計數
void GBA_UpdateCycles(GBA_Core *core, InstructionResult *result) {
    uint32_t actual_cycles = GBA_CalculateActualCycles(core, result);
    
    core->cpu.cycles.total += actual_cycles;
    core->cpu.cycles.this_frame += actual_cycles;
    core->cpu.cycles.instruction = actual_cycles;
    
    // 分支時清空預取緩衝區
    if (result->branch_taken || result->pipeline_flush) {
        GBA_PrefetchClear(core);
    }
}
```

## 各指令實際週期示例（基於實際記憶體區域）

### 在 BIOS 執行 (0x0000000-0x0003FFF)
```
指令: ADD r0, r1, r2    (ARM Data Processing)
基礎: 1S
實際: 1 cycle           (BIOS 快速訪問)
```

### 在 IWRAM 執行 (0x3000000-0x3007FFF)
```
指令: LDR r0, [r1]     (ARM Load)
基礎: 1S + 1N + 1I
實際: 1 + 1 + 1 = 3 cycles  (IWRAM 所有訪問都是1 cycle)
```

### 在 EWRAM 執行 (0x2000000-0x203FFFF)
```
指令: LDR r0, [r1]     (ARM Load, 32位)
基礎: 1S + 1N + 1I
實際: 3 + 6 + 1 = 10 cycles
      S-fetch=3, N-load(32bit)=3+3=6, I-transfer=1
```

### 在 ROM 執行 (0x8000000+, WS0預設配置)
```
指令: B label          (ARM Branch)
基礎: 2S + 1N
實際: 3 + 3 + 5 = 11 cycles (無預取)
      S-fetch=3, S-fetch=3, N-fetch=5

指令: SUB r0, r0, #1   (ARM Data Processing)  
基礎: 1S
實際: 3 cycles         (ROM Sequential = 3)
```

### THUMB 模式在 ROM
```
指令: MOVS r0, #0      (THUMB Move)
基礎: 1S
實際: 3 cycles         (ROM Sequential = 3)

指令: BL label         (THUMB Long Branch with Link, 2條指令)
基礎: 1S + (2S + 1N)
實際: 3 + 3 + 3 + 5 = 14 cycles
      第一條=3, 第二條 S-fetch=3, S-fetch=3, N-fetch=5
```

## 效能最佳化建議

### 1. 使用 IWRAM 存放關鍵程式碼
- IWRAM 訪問速度最快 (1 cycle)
- 適合放置迴圈和頻繁呼叫的函式
- 空間有限 (32KB)

### 2. 使用 EWRAM 存放資料
- 比 ROM 快，但比 IWRAM 慢
- 適合大陣列和緩衝區
- 16位訪問 = 3 cycles，32位訪問 = 6 cycles

### 3. 配置 WAITCNT 最佳化 ROM 訪問 (根據 GBATEK)
```c
// 商業卡帶常用配置 (來自 GBATEK)
#define WAITCNT_COMMON  0x4317
// Bit 0-1:   SRAM Wait = 3 → 8 cycles
// Bit 2-3:   WS0 First = 3 → 8 cycles  
// Bit 4:     WS0 Second = 1 → 1 cycle
// Bit 5-6:   WS1 First = 0 → 4 cycles
// Bit 7:     WS1 Second = 1 → 1 cycle
// Bit 8-9:   WS2 First = 0 → 4 cycles
// Bit 10:    WS2 Second = 0 → 8 cycles
// Bit 14:    Prefetch = 1 (enabled)
// 實際: WS0=8,1 clks; WS1=4,1 clks; WS2=4,8 clks; SRAM=8 clks

// 最快的 ROM 配置 (僅用於測試)
#define WAITCNT_FASTEST 0x4016
// WS0=2,1 clks; WS1=2,1 clks; WS2=2,1 clks; SRAM=2 clks; Prefetch=On

GBA_WriteIO16(core, 0x4000204, WAITCNT_COMMON);
```

### 4. 啟用 GamePak Prefetch
- 對順序程式碼執行有顯著提升
- 分支會清空緩衝區
- 適合主迴圈和順序執行的程式碼

### 5. 在 VBlank 期間訪問 VRAM
- 避免與 PPU 訪問衝突
- 在 H-Blank 或 V-Blank 期間 DMA 傳輸

## 參考資料

### 官方文件
- **ARM7TDMI Data Sheet** - Chapter 6: Memory Interface
- **ARM7TDMI Data Sheet** - Chapter 10: Instruction Cycle Operations

### 社群文件  
- **GBATEK** (https://problemkaputt.de/gbatek.htm) - 主要參考來源
  - GBA System Control - WAITCNT Register (0x4000204)
  - GBA Memory Map - Address Bus Width and CPU Read/Write Access Widths
  - GBA GamePak Prefetch - Detailed prefetch buffer behavior
  - ARM CPU Instruction Cycle Times - Complete cycle timing table
  
### 從 gbatek.html 提取的關鍵資訊 (2024)

**記憶體訪問週期表** (Lines 502-511):
- 完整的匯流排寬度、讀寫支援、週期時間資訊
- 明確說明 "1 clock cycle PLUS the number of waitstates"
- Video memory 訪問可能 +1 cycle (CPU-PPU conflict)

**WAITCNT 暫存器** (Lines 3387-3417):
- 準確的位定義和預設值 (啟動預設: 0000h)
- 商業卡帶常用值: 4317h
- WS1/WS2 的 Second Access 與 WS0 不同 (4/1 和 8/1)

**Prefetch Buffer** (Lines 3498-3525):
- 容量: 最多 8 個 16-bit 值
- 僅對 GamePak ROM opcodes 有效
- Prefetch Disable Bug: 1S → 1N

**128K 邊界** (Lines 3422-3424):
- GBA 強制在 128K 塊邊界使用 non-sequential timing

**ARM Instruction Cycles** (Lines 88761-88950):
- 完整的指令週期公式表
- N/S/I/C cycle 的詳細解釋
- Memory Waitstates 的說明

---

## 驗證和更新日誌

### 2024 更新 - 基於 gbatek.html 本地檔案驗證

**已驗證和修正的內容**:

1. ✅ **WAITCNT 暫存器位欄位** - 與 GBATEK Lines 3387-3417 完全一致
   - 預設啟動值: 0000h (之前文件有誤)
   - 商業卡帶常用: 4317h
   - WS0/WS1/WS2 Second Access 的不同值已修正

2. ✅ **記憶體訪問週期表** - 與 GBATEK Lines 502-511 完全一致
   - 新增了匯流排寬度、讀寫支援列
   - 明確了 "1 clock + waitstate" 公式
   - 新增了 CPU-PPU 衝突說明

3. ✅ **32-bit 訪問規則** - 根據 GBATEK Lines 3414-3417
   - "分成兩個 16-bit，第二個總是 sequential"
   - 更新了所有相關程式碼示例

4. ✅ **Prefetch Buffer** - 根據 GBATEK Lines 3498-3525
   - 容量: 8 個 16-bit 值
   - 僅 GamePak ROM opcodes 有效
   - Prefetch Enable 條件明確
   - Prefetch Disable Bug 說明

5. ✅ **128K 邊界強制 N-cycle** - GBATEK Lines 3422-3424
   - 新增了實現程式碼示例

6. ✅ **ARM 指令週期時間** - GBATEK Lines 88761-88950
   - 驗證了所有指令週期公式
   - 新增了 GBATEK 的詳細說明

**程式碼實現更新**:
- `GBA_GetROMWaitCycles()` - 使用正確的查詢表
- `GBA_GetSRAMWaitCycles()` - 使用正確的查詢表
- 新增了 128K 邊界檢測程式碼
- 新增了 Prefetch Buffer 結構定義

**參考來源**: `c:\Users\kd992\rGBA\gbatek.html` (本地檔案，94725 行)

---

### 其他參考資料

## 待實現功能清單

- [ ] 基礎 cycle type 系統 (N/S/I/C)
- [ ] GBA memory region 檢測
- [ ] WAITCNT 暫存器解析
- [ ] ROM waitstate 計算  
- [ ] SRAM waitstate 計算
- [ ] 32位訪問特殊處理
- [ ] Video memory conflict 檢測
- [ ] Prefetch buffer 模擬
- [ ] DMA cycle stealing
- [ ] 準確的 cycle 統計和顯示
