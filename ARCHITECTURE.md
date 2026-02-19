# GBA 模擬器 - 核心資料結構架構設計

## 📋 概述

這份文檔說明 GBA 模擬器最重要的打底資料結構設計。這些結構是整個模擬器的基礎，負責統合所有子系統的執行。

---

## 🏗️ 核心資料結構樹狀圖

```
Gba_System (完整系統)
├── Gba_Cpu (CPU 處理器)
│   ├── uint32_t Reg[16]           /* R0-R15 暫存器 */
│   ├── uint32_t CPSR              /* 狀態寄存器 */
│   ├── uint32_t fetchcache[3]     /* Pipeline 快取 */
│   └── enum DECODE_MODE dMode     /* ARM/THUMB 模式 */
│
├── GbaMem (內存系統)
│   ├── uint32_t *BIOS            /* 0x0000_0000 - 0x0000_3FFF (16 KB) */
│   ├── uint32_t *WRAM_board      /* 0x2000_0000 - 0x203F_FFFF (256 KB) */
│   ├── uint32_t *WRAM_chip       /* 0x3000_0000 - 0x3000_7FFF (32 KB) */
│   ├── uint32_t *IO_Reg          /* 0x4000_0000 - 0x4000_03FE (I/O) */
│   ├── uint32_t *Palette_RAM     /* 0x5000_0000 - 0x5000_03FF (1 KB) */
│   ├── uint32_t *VRAM            /* 0x6000_0000 - 0x6001_7FFF (96 KB) */
│   ├── uint32_t *OAM             /* 0x7000_0000 - 0x7000_03FF (1 KB) */
│   └── uint8_t *Game_ROM[3]      /* 0x8000_0000 ~ 0xE000_0000 (ROM 三個副本) */
│
├── Gba_InterruptSystem (中斷系統)
│   ├── uint16_t IF               /* 中斷請求旗標 (0x4000_0200) */
│   ├── uint16_t IE               /* 中斷啟用旗標 (0x4000_0202) */
│   ├── uint16_t IME              /* 中斷主控 (0x4000_0208) */
│   ├── uint32_t pending_interrupts /* 待處理中斷隊列 */
│   └── uint8_t vector_priority[14] /* 中斷優先級表 */
│
├── Gba_TimerSystem (定時器系統)
│   └── struct timers[4]
│       ├── uint16_t counter      /* 計數值 */
│       ├── uint16_t reload       /* 重載值 */
│       ├── uint16_t control      /* 控制機制 */
│       ├── uint8_t enabled       /* 是否啟用 */
│       ├── uint8_t cascade_enable/* 級聯控制 */
│       └── uint8_t prescaler     /* 預分頻器 */
│
├── Gba_PPUSystem (圖形系統)
│   ├── uint16_t DISPCNT          /* 顯示控制 (0x4000_0000) */
│   ├── uint16_t DISPSTAT         /* 顯示狀態 (0x4000_0004) */
│   ├── uint16_t VCOUNT           /* VCount 寄存器 */
│   ├── uint16_t BG_CNT[4]        /* 背景控制 */
│   ├── uint16_t BG_HOFS[4]       /* 水平偏移 */
│   ├── uint16_t BG_VOFS[4]       /* 垂直偏移 */
│   ├── uint32_t framebuffer[240*160] /* 24-bit 幀緩衝 */
│   ├── uint16_t scanline_cycle   /* 掃描線計時器 */
│   ├── uint16_t current_scanline /* 當前掃描線編號 */
│   └── uint8_t in_vblank/in_hblank
│
└── DMA (直接內存存取)
    └── DMA_CHANNEL DMAX[4]
        ├── uint32_t DMASAD      /* 來源地址 */
        ├── uint32_t DMADAD      /* 目標地址 */
        └── uint32_t DMACNT      /* 控制寄存器 */
```

---

## 🔄 執行流程圖

```
主程序 main()
  │
  ├─→ GbaSystem_Init()
  │   ├─ 分配內存
  │   ├─ 初始化所有子系統
  │   └─ 設置初始 CPU 狀態
  │
  └─→ 主循環 (每幀)
      │
      └─→ GbaSystem_RunFrame()
          │
          ├─→ 循環直到達到 280896 cycles
          │   │
          │   ├─→ CpuExecute(chunk_cycles)
          │   │   └─ 執行 ARM/THUMB 指令
          │   │
          │   ├─→ GbaSystem_UpdatePPU(chunk)
          │   │   ├─ 更新掃描線進度
          │   │   ├─ 檢查 V-Blank/H-Blank
          │   │   └─ 觸發相應中斷
          │   │
          │   ├─→ GbaSystem_UpdateTimers(chunk)
          │   │   ├─ 遞增計數器
          │   │   ├─ 檢查溢出
          │   │   └─ 觸發計時器中斷
          │   │
          │   └─→ GbaSystem_CheckInterrupts()
          │       └─ 取得最高優先級中斷
          │
          └─ 幀完成，準備下一幀
```

---

## 📊 時序常數

GBA 的時序非常精確，這些常數必須準確：

```c
CYCLES_PER_FRAME    = 280,896  cycles  /* 59.73 FPS */
CYCLES_PER_SCANLINE = 1,232    cycles  /* 一條掃描線 */
CYCLES_PER_HBLANK   = 272      cycles  /* H-Blank 期間 */
H_PIXELS            = 240      pixels
V_PIXELS            = 160      pixels
VDRAW_LINES         = 160      lines   /* 可見區域 */
VBLANK_LINES        = 68       lines   /* V-Blank 期間 */
```

掃描線時序：
```
[繪製 960 cycles] [H-Blank 272 cycles] = 1,232 cycles per line

幀時序：
[160 可見行] = 160 × 1,232 = 197,120 cycles
[68 Blank 行] = 68 × 1,232 = 83,776 cycles
────────────────────────────
總共：280,896 cycles ≈ 59.73 FPS
```

---

## 🎯 中斷系統架構

中斷優先級順序（由高到低）：

```
優先級   中斷類型           位    地址              觸發條件
═══════════════════════════════════════════════════════════════
0       V-Blank           0     0x4000_0200      VCount = 160
1       Timer 0           3     0x4000_0100      計數器溢出
2       DMA 0             8     0x40000_B8A      DMA 完成
3       H-Blank           1     0x4000_0004      掃描線 960+ cycles
4       V-Count Match     2     0x4000_0006      VCount 比對
5       Timer 1           4                     計數器溢出
6       DMA 1             9                     DMA 完成
7       Timer 2           5                     計數器溢出
8       DMA 2             10                    DMA 完成
9       Timer 3           6                     計數器溢出
10      DMA 3             11                    DMA 完成
11      Serial            7                     通信完成
12      Keypad            12                    按鍵事件
13      Cartridge         13                    遊戲卡帶
```

中斷控制寄存器：
```
0x4000_0200 (IF - Interrupt Request)
  bit 0:   V-Blank (寫 1 清除)
  bit 1-7: 其他中斷標誌
  bit 8-13: DMA/其他中斷

0x4000_0202 (IE - Interrupt Enable)
  同上，用於啟用/禁用中斷

0x4000_0208 (IME - Interrupt Master Enable)
  bit 0: 1 = 啟用所有中斷；0 = 禁用
```

---

## 💾 內存映射

```
0x0000_0000  ┌─────────────────┐
             │   BIOS (16 KB)  │
0x0000_3FFF  ├─────────────────┤
             │  (未使用)       │
0x2000_0000  ├─────────────────┐
             │ WRAM 板載(256KB) │
0x203F_FFFF  ├─────────────────┤
             │  (未使用)       │
0x3000_0000  ├─────────────────┐
             │ WRAM 晶片 (32KB) │
0x3000_7FFF  ├─────────────────┤
             │  (未使用)       │
0x4000_0000  ├─────────────────┐
             │   I/O 寄存器    │  ← LCD, Timer, DMA, IF, IE 等
0x4000_03FF  ├─────────────────┤
             │  (未使用)       │
0x5000_0000  ├─────────────────┐
             │ Palette RAM (1KB)│  ← 顏色調色板
0x5000_03FF  ├─────────────────┤
             │  (未使用)       │
0x6000_0000  ├─────────────────┐
             │ VRAM (96 KB)    │  ← 背景/窗口/精靈數據
0x6001_7FFF  ├─────────────────┤
             │  (未使用)       │
0x7000_0000  ├─────────────────┐
             │ OAM (1 KB)      │  ← 精靈屬性
0x7000_03FF  ├─────────────────┤
             │  (未使用)       │
0x8000_0000  ├─────────────────┐
             │ Game ROM1       │  ─ 副本1
0x9FFF_FFFF  ├─────────────────┤
0xA000_0000  │ Game ROM2       │  ─ 副本2
0xBFFF_FFFF  ├─────────────────┤
0xC000_0000  │ Game ROM3       │  ─ 副本3
0xDFFF_FFFF  ├─────────────────┤
0xE000_0000  │ Game ROM SRAM   │  ─ SRAM 區域
0xFFFE_FFFF  └─────────────────┘
```

---

## 🔌 使用範例

### 初始化系統
```c
Gba_System gba;
GbaSystem_Init(&gba);
```

### 主執行循環
```c
while (game_running) {
    uint32_t cycles = GbaSystem_RunFrame(&gba);
    
    // 繪製幀緩衝
    render_framebuffer(gba.ppu.framebuffer);
    
    // 處理輸入等
    process_input(&gba);
}
```

### 中斷操作
```c
// 要求 V-Blank 中斷
Interrupt_Request(&gba, INT_VBLANK);

// 啟用定時器中斷
Interrupt_Enable(&gba, INT_TIMER0);

// 設置全局中斷啟用
Interrupt_SetMasterEnable(&gba, 1);
```

---

## 📝 實現檢查清單

- [x] `gba_system.h` - 核心資料結構定義
- [x] `gba_system.c` - 系統驅動邏輯
- [x] `gba_interrupt.h` - 中斷系統實現
- [ ] 連接到 `main.c` 的主循環
- [ ] 完成 CPU 指令執行
- [ ] 完成 PPU 幀緩衝實際渲染
- [ ] 完成定時器計數邏輯
- [ ] 完成 DMA 傳輸邏輯

---

## 🎓 設計重點

1. **統一驅動**：`Gba_System` 包含所有子系統，便於統一管理和同步
2. **精確時序**：週期計數與各子系統同步，確保準確性
3. **優先級中斷**：中斷系統支持優先級排序，符合 GBA 硬件行為
4. **模塊化**：各個子系統可獨立更新，便於維護和優化
5. **狀態可查**：提供統計函數查詢系統狀態用於調試
