/**
 * GBA 模擬器核心架構 - 現代化設計
 * 
 * 設計原則：
 * 1. 無全局變數 - 所有狀態封裝在結構中
 * 2. 清晰的所有權 - 明確的資源管理
 * 3. 統一接口 - 一致的函數簽名
 * 4. 高性能 - 內聯函數、查找表、緩存友好
 * 5. 可測試性 - 易於單元測試和調試
 */

#ifndef GBA_CORE_H
#define GBA_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * 前向聲明
 * ============================================================================ */
typedef struct GBA_Core GBA_Core;
typedef struct GBA_CPU GBA_CPU;
typedef struct GBA_Memory GBA_Memory;
typedef struct GBA_PPU GBA_PPU;
typedef struct GBA_Audio GBA_Audio;
typedef struct GBA_Timer GBA_Timer;
typedef struct GBA_DMA GBA_DMA;
typedef struct GBA_Interrupt GBA_Interrupt;

/* ============================================================================
 * CPU 執行模式
 * ============================================================================ */
typedef enum {
    CPU_MODE_ARM   = 0,
    CPU_MODE_THUMB = 1
} CPU_ExecutionMode;

typedef enum {
    CPU_STATE_USER       = 0x10,
    CPU_STATE_FIQ        = 0x11,
    CPU_STATE_IRQ        = 0x12,
    CPU_STATE_SUPERVISOR = 0x13,
    CPU_STATE_ABORT      = 0x17,
    CPU_STATE_UNDEFINED  = 0x1B,
    CPU_STATE_SYSTEM     = 0x1F
} CPU_PrivilegeMode;

/* ============================================================================
 * 指令執行結果
 * ============================================================================ */
typedef struct {
    uint8_t  cycles;          // 指令消耗的周期數
    bool     branch_taken;    // 是否發生分支
    bool     pipeline_flush;  // 是否需要刷新流水線
    bool     mode_changed;    // 是否切換了 ARM/THUMB 模式
} InstructionResult;

/* ============================================================================
 * CPU 寄存器組
 * ============================================================================ */
typedef struct {
    // 當前可見的寄存器 (R0-R14)
    uint32_t r[15];
    
    // 程序計數器 (R15) - 帶 pipeline offset
    // ARM 模式: pc = exec_addr + 8
    // THUMB 模式: pc = exec_addr + 4
    uint32_t pc;
    
    // 當前執行地址 (實際執行指令的記憶體位置)
    uint32_t exec_addr;
    
    // 當前程序狀態寄存器
    uint32_t cpsr;
    
    // 銀行寄存器 (Banked Registers)
    struct {
        uint32_t r8_fiq, r9_fiq, r10_fiq, r11_fiq, r12_fiq;
        uint32_t r13_fiq, r14_fiq;  // SP_fiq, LR_fiq
        uint32_t spsr_fiq;
    } fiq;
    
    struct {
        uint32_t r13, r14;  // SP, LR
        uint32_t spsr;
    } irq, svc, abt, und;

    // User/System 模式寄存器快照
    struct {
        uint32_t r8_usr, r9_usr, r10_usr, r11_usr, r12_usr;
        uint32_t r13_usr, r14_usr;
    } usr;
    
    // 流水線
    uint32_t pipeline[3];  // [0]=execute, [1]=decode, [2]=fetch
    
    // 執行狀態
    CPU_ExecutionMode exec_mode;
    CPU_PrivilegeMode priv_mode;
    
} GBA_CPU_Registers;

/* ============================================================================
 * CPU 核心
 * ============================================================================ */
struct GBA_CPU {
    GBA_CPU_Registers regs;
    
    // 周期追蹤
    struct {
        uint64_t total;           // 總執行周期
        uint32_t this_frame;      // 本幀已執行周期
        uint32_t instruction;     // 當前指令周期
    } cycles;
    
    // 狀態標誌
    struct {
        bool halted;              // CPU 暫停
        bool irq_line;            // IRQ 線狀態
        bool fiq_line;            // FIQ 線狀態
    } state;
    
    // 調試信息 (可選，Release 版本可移除)
    #ifdef GBA_DEBUG
    struct {
        uint32_t last_pc;
        uint32_t instruction_count;
        char     last_mnemonic[32];
    } debug;
    #endif
};

/* ============================================================================
 * 記憶體區域定義
 * ============================================================================ */
typedef enum {
    MEM_REGION_BIOS        = 0,   // 0x0000_0000 - 0x0000_3FFF (16 KB)
    MEM_REGION_EWRAM       = 1,   // 0x0200_0000 - 0x0203_FFFF (256 KB)
    MEM_REGION_IWRAM       = 2,   // 0x0300_0000 - 0x0300_7FFF (32 KB)
    MEM_REGION_IO          = 3,   // 0x0400_0000 - 0x0400_03FF
    MEM_REGION_PALETTE     = 4,   // 0x0500_0000 - 0x0500_03FF (1 KB)
    MEM_REGION_VRAM        = 5,   // 0x0600_0000 - 0x0601_7FFF (96 KB)
    MEM_REGION_OAM         = 6,   // 0x0700_0000 - 0x0700_03FF (1 KB)
    MEM_REGION_ROM_WS0     = 7,   // 0x0800_0000 - 0x09FF_FFFF
    MEM_REGION_ROM_WS1     = 8,   // 0x0A00_0000 - 0x0BFF_FFFF
    MEM_REGION_ROM_WS2     = 9,   // 0x0C00_0000 - 0x0DFF_FFFF
    MEM_REGION_SRAM        = 10,  // 0x0E00_0000 - 0x0E00_FFFF
    MEM_REGION_COUNT       = 11
} MemoryRegion;

/* ============================================================================
 * 記憶體系統
 * ============================================================================ */
struct GBA_Memory {
    // 實際記憶體區域
    uint8_t *bios;           // 16 KB
    uint8_t *ewram;          // 256 KB (External Work RAM)
    uint8_t *iwram;          // 32 KB (Internal Work RAM)
    uint8_t *io_registers;   // 1 KB
    uint8_t *palette;        // 1 KB
    uint8_t *vram;           // 96 KB
    uint8_t *oam;            // 1 KB
    uint8_t *rom;            // 最大 32 MB
    uint8_t *sram;           // 64 KB
    
    // ROM 資訊
    size_t rom_size;
    size_t sram_size;
    
    // 等待狀態設定 (wait states)
    struct {
        uint8_t sram;
        uint8_t ws0_n, ws0_s;  // Non-sequential, Sequential
        uint8_t ws1_n, ws1_s;
        uint8_t ws2_n, ws2_s;
    } wait_states;
    
    // DMA 正在訪問的區域 (用於總線仲裁)
    bool dma_active[4];
    
    // 訪問統計 (調試用)
    #ifdef GBA_DEBUG
    struct {
        uint64_t reads[MEM_REGION_COUNT];
        uint64_t writes[MEM_REGION_COUNT];
    } stats;
    #endif
};

/* ============================================================================
 * 中斷系統
 * ============================================================================ */
typedef enum {
    IRQ_VBLANK   = (1 << 0),
    IRQ_HBLANK   = (1 << 1),
    IRQ_VCOUNT   = (1 << 2),
    IRQ_TIMER0   = (1 << 3),
    IRQ_TIMER1   = (1 << 4),
    IRQ_TIMER2   = (1 << 5),
    IRQ_TIMER3   = (1 << 6),
    IRQ_SERIAL   = (1 << 7),
    IRQ_DMA0     = (1 << 8),
    IRQ_DMA1     = (1 << 9),
    IRQ_DMA2     = (1 << 10),
    IRQ_DMA3     = (1 << 11),
    IRQ_KEYPAD   = (1 << 12),
    IRQ_GAMEPAK  = (1 << 13)
} InterruptType;

struct GBA_Interrupt {
    uint16_t IE;   // Interrupt Enable
    uint16_t IF;   // Interrupt Request Flags
    uint16_t IME;  // Interrupt Master Enable
    
    // 待處理的中斷 (按優先級排序)
    uint16_t pending;
    
    // 中斷處理中標誌
    bool in_handler;
};

/* ============================================================================
 * PPU 系統
 * ============================================================================ */
struct GBA_PPU {
    // 顯示控制
    uint16_t DISPCNT;
    uint16_t DISPSTAT;
    uint16_t VCOUNT;
    
    // 背景控制
    uint16_t BG_CNT[4];
    uint16_t BG_HOFS[4];
    uint16_t BG_VOFS[4];
    
    // 當前掃描線狀態
    uint16_t current_scanline;
    uint32_t scanline_cycles;
    
    // 幀緩衝 (BGRA8888 格式)
    uint32_t *framebuffer;  // 240 × 160 × 4 bytes
    
    // 狀態標誌
    bool in_vblank;
    bool in_hblank;
    
    // 渲染緩存 (用於加速)
    struct {
        bool bg_enabled[4];
        bool obj_enabled;
        uint8_t video_mode;
    } cache;
};

/* ============================================================================
 * 定時器
 * ============================================================================ */
struct GBA_Timer {
    struct {
        uint16_t counter;
        uint16_t reload;
        uint16_t control;
        uint32_t internal_counter;
        bool     enabled;
        bool     irq_enable;
        uint8_t  prescaler;
        bool     count_up;
    } timers[4];
};

/* ============================================================================
 * DMA 控制器
 * ============================================================================ */
struct GBA_DMA {
    struct {
        uint32_t source;
        uint32_t dest;
        uint16_t count;
        uint16_t control;
        
        // 內部狀態
        uint32_t internal_source;
        uint32_t internal_dest;
        uint16_t internal_count;
        bool     enabled;
        bool     repeat;
    } channels[4];
    
    // 當前活躍的 DMA 通道 (-1 表示無)
    int8_t active_channel;
};

/* ============================================================================
 * 核心系統 (統一所有子系統)
 * ============================================================================ */
struct GBA_Core {
    // 子系統
    GBA_CPU       cpu;
    GBA_Memory    memory;
    GBA_PPU       ppu;
    GBA_Interrupt interrupt;
    GBA_Timer     timer;
    GBA_DMA       dma;
    
    // 全局狀態
    struct {
        uint64_t frame_number;
        bool     running;
        bool     paused;
    } state;
    
    // 事件調度器 (可選，用於精確時序)
    #ifdef GBA_USE_SCHEDULER
    struct {
        uint32_t next_event_cycle;
        void (*event_handlers[16])(GBA_Core *core);
        uint32_t event_cycles[16];
        uint8_t  event_count;
    } scheduler;
    #endif
};

/* ============================================================================
 * 核心 API
 * ============================================================================ */

/**
 * 初始化 GBA 核心
 * @return 成功返回指向 GBA_Core 的指針，失敗返回 NULL
 */
GBA_Core* GBA_CoreCreate(void);

/**
 * 釋放 GBA 核心資源
 */
void GBA_CoreDestroy(GBA_Core *core);

/**
 * 重置 GBA 到初始狀態
 */
void GBA_CoreReset(GBA_Core *core);

/**
 * 載入 BIOS
 * @param data BIOS 數據 (必須 16 KB)
 * @return 成功返回 0，失敗返回 -1
 */
int GBA_CoreLoadBIOS(GBA_Core *core, const uint8_t *data, size_t size);

/**
 * 載入 ROM
 * @param data ROM 數據
 * @param size ROM 大小
 * @return 成功返回 0，失敗返回 -1
 */
int GBA_CoreLoadROM(GBA_Core *core, const uint8_t *data, size_t size);

/**
 * 執行一幀 (280,896 周期)
 * @return 實際執行的周期數
 */
uint32_t GBA_CoreRunFrame(GBA_Core *core);
void GBA_CoreDumpState(const GBA_Core *core);

/**
 * 執行指定數量的周期
 * @param cycles 要執行的周期數
 * @return 實際執行的周期數
 */
uint32_t GBA_CoreRunCycles(GBA_Core *core, uint32_t cycles);

/**
 * 獲取幀緩衝指針
 * @return 指向 240×160 BGRA8888 幀緩衝的指針
 */
const uint32_t* GBA_CoreGetFramebuffer(const GBA_Core *core);

/* ============================================================================
 * CPU API
 * ============================================================================ */

/**
 * 執行單條指令
 * @return 指令執行結果
 */
InstructionResult GBA_CPUStep(GBA_Core *core);

/**
 * 檢查並處理中斷
 */
void GBA_CPUCheckInterrupts(GBA_Core *core);

/**
 * 切換 CPU 特權模式
 */
void GBA_CPU_SwitchMode(GBA_Core *core, CPU_PrivilegeMode new_mode);

/**
 * 刷新流水線
 */
void GBA_CPUFlushPipeline(GBA_Core *core);

/* ============================================================================
 * 記憶體 API
 * ============================================================================ */

/**
 * 記憶體讀取 (自動處理等待狀態)
 */
uint32_t GBA_MemoryRead32(GBA_Core *core, uint32_t address);
uint16_t GBA_MemoryRead16(GBA_Core *core, uint32_t address);
uint8_t  GBA_MemoryRead8(GBA_Core *core, uint32_t address);

/**
 * 記憶體寫入
 */
void GBA_MemoryWrite32(GBA_Core *core, uint32_t address, uint32_t value);
void GBA_MemoryWrite16(GBA_Core *core, uint32_t address, uint16_t value);
void GBA_MemoryWrite8(GBA_Core *core, uint32_t address, uint8_t value);

/**
 * 獲取記憶體區域指針 (快速訪問，但要注意邊界)
 */
uint8_t* GBA_MemoryGetRegionPointer(GBA_Core *core, MemoryRegion region);

/**
 * 計算記憶體訪問周期 (包含等待狀態)
 */
uint8_t GBA_MemoryGetAccessCycles(GBA_Core *core, uint32_t address, bool sequential);

/* ============================================================================
 * 中斷 API
 * ============================================================================ */

/**
 * 請求中斷
 */
void GBA_InterruptRequest(GBA_Core *core, InterruptType type);

/**
 * 清除中斷
 */
void GBA_InterruptClear(GBA_Core *core, InterruptType type);

/**
 * 檢查是否有待處理的中斷
 */
bool GBA_InterruptPending(const GBA_Core *core);

/**
 * 獲取最高優先級的待處理中斷
 */
InterruptType GBA_InterruptGetHighestPriority(const GBA_Core *core);

/* ============================================================================
 * PPU API
 * ============================================================================ */

/**
 * 更新 PPU (執行指定周期)
 */
void GBA_PPUUpdate(GBA_Core *core, uint32_t cycles);

/**
 * 渲染當前幀
 */
void GBA_PPURenderFrame(GBA_Core *core);

/* ============================================================================
 * 定時器 API
 * ============================================================================ */

/**
 * 更新所有定時器
 */
void GBA_TimerUpdate(GBA_Core *core, uint32_t cycles);

/* ============================================================================
 * DMA API
 * ============================================================================ */

/**
 * 執行 DMA 傳輸
 */
void GBA_DMAUpdate(GBA_Core *core);

/**
 * 檢查 DMA 是否活躍
 */
bool GBA_DMAIsActive(const GBA_Core *core);

/* ============================================================================
 * 內聯輔助函數 (性能優化)
 * ============================================================================ */

// 獲取 CPSR 標誌位
static inline bool GBA_CPU_GetFlag_N(const GBA_CPU *cpu) {
    return (cpu->regs.cpsr >> 31) & 1;
}

static inline bool GBA_CPU_GetFlag_Z(const GBA_CPU *cpu) {
    return (cpu->regs.cpsr >> 30) & 1;
}

static inline bool GBA_CPU_GetFlag_C(const GBA_CPU *cpu) {
    return (cpu->regs.cpsr >> 29) & 1;
}

static inline bool GBA_CPU_GetFlag_V(const GBA_CPU *cpu) {
    return (cpu->regs.cpsr >> 28) & 1;
}

// 設置 CPSR 標誌位
static inline void GBA_CPU_SetFlag_N(GBA_CPU *cpu, bool value) {
    if (value) cpu->regs.cpsr |= (1U << 31);
    else       cpu->regs.cpsr &= ~(1U << 31);
}

static inline void GBA_CPU_SetFlag_Z(GBA_CPU *cpu, bool value) {
    if (value) cpu->regs.cpsr |= (1U << 30);
    else       cpu->regs.cpsr &= ~(1U << 30);
}

static inline void GBA_CPU_SetFlag_C(GBA_CPU *cpu, bool value) {
    if (value) cpu->regs.cpsr |= (1U << 29);
    else       cpu->regs.cpsr &= ~(1U << 29);
}

static inline void GBA_CPU_SetFlag_V(GBA_CPU *cpu, bool value) {
    if (value) cpu->regs.cpsr |= (1U << 28);
    else       cpu->regs.cpsr &= ~(1U << 28);
}

// 快速記憶體區域判斷
static inline MemoryRegion GBA_MemoryGetRegion(uint32_t address) {
    switch (address >> 24) {
        case 0x00: return MEM_REGION_BIOS;
        case 0x02: return MEM_REGION_EWRAM;
        case 0x03: return MEM_REGION_IWRAM;
        case 0x04: return MEM_REGION_IO;
        case 0x05: return MEM_REGION_PALETTE;
        case 0x06: return MEM_REGION_VRAM;
        case 0x07: return MEM_REGION_OAM;
        case 0x08:
        case 0x09: return MEM_REGION_ROM_WS0;
        case 0x0A:
        case 0x0B: return MEM_REGION_ROM_WS1;
        case 0x0C:
        case 0x0D: return MEM_REGION_ROM_WS2;
        case 0x0E:
        case 0x0F: return MEM_REGION_SRAM;
        default:   return MEM_REGION_BIOS;  // 未映射區域
    }
}

#endif // GBA_CORE_H
