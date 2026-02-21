/**
 * GBA 中斷系統實現
 * 處理所有 14 種中斷的請求、啟用、優先順序和派發
 */

#ifndef GBA_INTERRUPT_H
#define GBA_INTERRUPT_H

#include <stdint.h>
#include "gba_system.h"

/* ============================================================================
 * 中斷定義與常數
 * ============================================================================ */

// IE/IF 暫存器的中斷位定義
#define INT_VBLANK      (1 <<  0)      // V-Blank 中斷
#define INT_HBLANK      (1 <<  1)      // H-Blank 中斷
#define INT_VCOUNT      (1 <<  2)      // V-Count 比對中斷
#define INT_TIMER0      (1 <<  3)      // Timer 0 溢位
#define INT_TIMER1      (1 <<  4)      // Timer 1 溢位
#define INT_TIMER2      (1 <<  5)      // Timer 2 溢位
#define INT_TIMER3      (1 <<  6)      // Timer 3 溢位
#define INT_SERIAL      (1 <<  7)      // 序列通訊中斷
#define INT_DMA0        (1 <<  8)      // DMA 0 完成
#define INT_DMA1        (1 <<  9)      // DMA 1 完成
#define INT_DMA2        (1 << 10)      // DMA 2 完成
#define INT_DMA3        (1 << 11)      // DMA 3 完成
#define INT_KEYPAD      (1 << 12)      // 按鍵中斷
#define INT_CARTRIDGE   (1 << 13)      // 遊戲卡帶中斷

// 中斷優先順序 (數值越小優先順序越高)
typedef enum {
    PRIORITY_VBLANK = 0,
    PRIORITY_TIMER0 = 1,
    PRIORITY_DMA0 = 2,
    PRIORITY_HBLANK = 3,
    PRIORITY_VCOUNT = 4,
    PRIORITY_TIMER1 = 5,
    PRIORITY_DMA1 = 6,
    PRIORITY_TIMER2 = 7,
    PRIORITY_DMA2 = 8,
    PRIORITY_TIMER3 = 9,
    PRIORITY_DMA3 = 10,
    PRIORITY_SERIAL = 11,
    PRIORITY_KEYPAD = 12,
    PRIORITY_CARTRIDGE = 13
} Gba_InterruptPriority;

/* ============================================================================
 * 中斷操作函式
 * ============================================================================ */

/**
 * 要求一個中斷
 * @param system GBA系統指標
 * @param interrupt_bit 中斷位 (使用 INT_* 常數)
 */
void Interrupt_Request(Gba_System *system, uint16_t interrupt_bit);

/**
 * 清除一個中斷請求
 * @param system GBA系統指標
 * @param interrupt_bit 中斷位
 */
void Interrupt_Clear(Gba_System *system, uint16_t interrupt_bit);

/**
 * 啟用特定中斷
 * @param system GBA系統指標
 * @param interrupt_bit 中斷位
 */
void Interrupt_Enable(Gba_System *system, uint16_t interrupt_bit);

/**
 * 禁用特定中斷
 * @param system GBA系統指標
 * @param interrupt_bit 中斷位
 */
void Interrupt_Disable(Gba_System *system, uint16_t interrupt_bit);

/**
 * 設定中斷主控開關
 * @param system GBA系統指標
 * @param enable 1=啟用，0=禁用
 */
void Interrupt_SetMasterEnable(Gba_System *system, uint8_t enable);

/**
 * 獲取最高優先順序的待處理中斷
 * @param system GBA系統指標
 * @return 最高優先順序中斷位，如無則返回0
 */
uint16_t Interrupt_GetHighestPending(Gba_System *system);

/**
 * 派發中斷給CPU處理
 * @param system GBA系統指標
 * @param interrupt_bit 要派發的中斷位
 * @return 成功返回0，失敗返回非0
 */
int Interrupt_Dispatch(Gba_System *system, uint16_t interrupt_bit);

/**
 * 設定中斷優先順序
 * @param system GBA系統指標
 * @param interrupt_index 中斷索引 (0-13)
 * @param priority 優先順序值 (越小優先順序越高)
 */
void Interrupt_SetPriority(Gba_System *system, uint8_t interrupt_index, 
                           uint8_t priority);

/**
 * 取得中斷的向量地址 (中斷處理程式的地址)
 * @param interrupt_bit 中斷位
 * @return 中斷向量地址
 */
uint32_t Interrupt_GetVector(uint16_t interrupt_bit);

/**
 * 讀取中斷控制暫存器 (I/O 讀取介面)
 * @param system GBA系統指標
 * @param address 暫存器地址 (0x4000200 ~ 0x4000202)
 * @return 暫存器值
 */
uint16_t Interrupt_ReadReg(Gba_System *system, uint32_t address);

/**
 * 寫入中斷控制暫存器 (I/O 寫入介面)
 * @param system GBA系統指標
 * @param address 暫存器地址 (0x4000200 ~ 0x4000202)
 * @param value 要寫入的值
 */
void Interrupt_WriteReg(Gba_System *system, uint32_t address, uint16_t value);

#endif // GBA_INTERRUPT_H


/**
 * 中斷系統實現程式碼
 * ============================================================================
 */

#include <stdio.h>

/* 中斷向量地址表 */
static const uint32_t interrupt_vectors[14] = {
    0x18,   // V-Blank
    0x1C,   // H-Blank
    0x20,   // V-Count
    0x24,   // Timer 0
    0x28,   // Timer 1
    0x2C,   // Timer 2
    0x30,   // Timer 3
    0x34,   // Serial
    0x38,   // DMA 0
    0x3C,   // DMA 1
    0x40,   // DMA 2
    0x44,   // DMA 3
    0x48,   // Keypad
    0x4C    // Cartridge
};

void Interrupt_Request(Gba_System *system, uint16_t interrupt_bit) {
    if (!system) return;
    
    system->interrupt.IF |= interrupt_bit;
    
    // 除錯輸出
    if (interrupt_bit == INT_VBLANK) printf("[INT] V-Blank 請求\n");
    else if (interrupt_bit == INT_HBLANK) printf("[INT] H-Blank 請求\n");
    else if (interrupt_bit == INT_VCOUNT) printf("[INT] V-Count 請求\n");
    else if (interrupt_bit & 0x78) printf("[INT] Timer 請求 (0x%X)\n", interrupt_bit);
    else if (interrupt_bit & 0xF00) printf("[INT] DMA 請求 (0x%X)\n", interrupt_bit);
}

void Interrupt_Clear(Gba_System *system, uint16_t interrupt_bit) {
    if (!system) return;
    system->interrupt.IF &= ~interrupt_bit;
}

void Interrupt_Enable(Gba_System *system, uint16_t interrupt_bit) {
    if (!system) return;
    system->interrupt.IE |= interrupt_bit;
}

void Interrupt_Disable(Gba_System *system, uint16_t interrupt_bit) {
    if (!system) return;
    system->interrupt.IE &= ~interrupt_bit;
}

void Interrupt_SetMasterEnable(Gba_System *system, uint8_t enable) {
    if (!system) return;
    system->interrupt.IME = enable ? 1 : 0;
    printf("[INT] 中斷主控: %s\n", enable ? "啟用" : "禁用");
}

uint16_t Interrupt_GetHighestPending(Gba_System *system) {
    if (!system) return 0;
    
    // 檢查有效的中斷 (IF & IE)
    uint16_t active = system->interrupt.IF & system->interrupt.IE;
    
    if (!active) return 0;
    
    // 根據優先順序返回最高優先順序的中斷
    // 優先順序順序: VBLANK > TIMER0 > DMA0 > HBLANK > ...
    uint16_t priority_order[] = {
        INT_VBLANK, INT_TIMER0, INT_DMA0, INT_HBLANK, INT_VCOUNT,
        INT_TIMER1, INT_DMA1, INT_TIMER2, INT_DMA2, INT_TIMER3,
        INT_DMA3, INT_SERIAL, INT_KEYPAD, INT_CARTRIDGE
    };
    
    for (int i = 0; i < 14; i++) {
        if (active & priority_order[i]) {
            return priority_order[i];
        }
    }
    
    return 0;
}

int Interrupt_Dispatch(Gba_System *system, uint16_t interrupt_bit) {
    if (!system || !system->cpu) return 1;
    
    printf("[INT] 派發中斷: 0x%04X\n", interrupt_bit);
    
    // TODO: 實現實際的中斷派發邏輯
    // 1. 儲存當前 PC 和 CPSR 到記憶體
    // 2. 設定 LR = 返回地址
    // 3. 設定 SPSR_irq = CPSR
    // 4. 設定 CPSR 為 IRQ mode
    // 5. 設定 PC 為中斷向量地址
    // 6. 禁用進一步的 IRQ (設定 CPSR 的 I flag)
    
    return 0;
}

void Interrupt_SetPriority(Gba_System *system, uint8_t interrupt_index, 
                           uint8_t priority) {
    if (!system || interrupt_index >= 14) return;
    system->interrupt.vector_priority[interrupt_index] = priority;
}

uint32_t Interrupt_GetVector(uint16_t interrupt_bit) {
    // 根據中斷位找到對應的向量索引
    for (int i = 0; i < 14; i++) {
        if ((1 << i) == interrupt_bit) {
            return interrupt_vectors[i];
        }
    }
    return 0;
}

uint16_t Interrupt_ReadReg(Gba_System *system, uint32_t address) {
    if (!system) return 0;
    
    switch (address) {
        case 0x4000200:  // IF (中斷請求)
            return system->interrupt.IF;
        case 0x4000202:  // IE (中斷啟用)
            return system->interrupt.IE;
        case 0x4000208:  // IME (中斷主控)
            return system->interrupt.IME;
        default:
            return 0;
    }
}

void Interrupt_WriteReg(Gba_System *system, uint32_t address, uint16_t value) {
    if (!system) return;
    
    switch (address) {
        case 0x4000200:  // IF - 寫 1 清除中斷
            system->interrupt.IF &= ~value;
            printf("[INT] IF 寫入: 清除 0x%04X，剩餘 0x%04X\n", value, system->interrupt.IF);
            break;
        case 0x4000202:  // IE - 設定中斷啟用
            system->interrupt.IE = value;
            printf("[INT] IE 寫入: 0x%04X\n", value);
            break;
        case 0x4000208:  // IME - 設定中斷主控
            system->interrupt.IME = value & 1;
            printf("[INT] IME 寫入: %d\n", value & 1);
            break;
    }
}
