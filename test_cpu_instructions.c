/**
 * GBA CPU 指令測試框架
 * 用於驗證 ARM 和 THUMB 指令的正確性
 */

#include "gba_core.h"
#include "gba_cpu_instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 測試統計 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 測試輔助宏 */
#define TEST_START(name) \
    do { \
        printf("\n=== 測試: %s ===\n", name); \
        tests_run++; \
    } while(0)

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            printf("  ❌ 失敗: %s\n", msg); \
            printf("     期望值: 0x%08X\n", (uint32_t)(expected)); \
            printf("     實際值: 0x%08X\n", (uint32_t)(actual)); \
            tests_failed++; \
            return false; \
        } \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            printf("  ❌ 失敗: %s\n", msg); \
            tests_failed++; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition, msg) \
    do { \
        if ((condition)) { \
            printf("  ❌ 失敗: %s\n", msg); \
            tests_failed++; \
            return false; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("  ✅ 通過\n"); \
        tests_passed++; \
        return true; \
    } while(0)

/* ============================================================================
 * 測試輔助函數
 * ============================================================================ */

/**
 * 創建測試用的 GBA Core
 */
GBA_Core* CreateTestCore() {
    GBA_Core *core = GBA_CoreCreate();
    if (!core) {
        fprintf(stderr, "無法創建測試用核心\n");
        return NULL;
    }
    
    // 初始化一些基本記憶體（避免訪問空指標）
    memset(core->memory.iwram, 0, 32 * 1024);
    
    // 設置一個簡單的測試環境
    core->cpu.regs.pc = 0x03000000;  // IWRAM
    core->cpu.regs.r[13] = 0x03007F00;  // SP
    
    return core;
}

/**
 * 重置 CPU 寄存器到初始狀態
 */
void ResetCPURegisters(GBA_Core *core) {
    memset(&core->cpu, 0, sizeof(GBA_CPU));
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.pc = 0x03000000;
    core->cpu.regs.r[13] = 0x03007F00;
}

/**
 * 設置 CPSR 標誌位（用於測試條件指令）
 */
void SetFlags(GBA_Core *core, bool N, bool Z, bool C, bool V) {
    uint32_t cpsr = core->cpu.regs.cpsr & 0x0FFFFFFF;
    if (N) cpsr |= (1U << 31);
    if (Z) cpsr |= (1U << 30);
    if (C) cpsr |= (1U << 29);
    if (V) cpsr |= (1U << 28);
    core->cpu.regs.cpsr = cpsr;
}

/* ============================================================================
 * ARM 數據處理指令測試
 * ============================================================================ */

bool Test_ARM_ADD() {
    TEST_START("ARM ADD");
    GBA_Core *core = CreateTestCore();
    
    // 設置初始值
    core->cpu.regs.r[0] = 10;
    core->cpu.regs.r[1] = 20;
    
    // 執行 ADD R2, R0, R1 (E0802001)
    uint32_t inst = 0xE0802001;
    InstructionResult result = ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[2], 30, "10 + 20 = 30");
    ASSERT_EQ(result.cycles, 1, "ADD 應消耗 1 週期");
    ASSERT_FALSE(result.branch_taken, "ADD 不應分支");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_SUB() {
    TEST_START("ARM SUB");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.r[0] = 50;
    core->cpu.regs.r[1] = 30;
    
    // SUB R2, R0, R1 (E0402001)
    uint32_t inst = 0xE0402001;
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[2], 20, "50 - 30 = 20");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_MOV() {
    TEST_START("ARM MOV");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.r[1] = 0x12345678;
    
    // MOV R0, R1 (E1A00001)
    uint32_t inst = 0xE1A00001;
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[0], 0x12345678, "R0 應等於 R1");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_MOV_Immediate() {
    TEST_START("ARM MOV 立即數");
    GBA_Core *core = CreateTestCore();
    
    // MOV R0, #42 (E3A0002A)
    uint32_t inst = 0xE3A0002A;
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[0], 42, "R0 應等於 42");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_CMP() {
    TEST_START("ARM CMP 設置標誌位");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.r[0] = 10;
    core->cpu.regs.r[1] = 10;
    
    // CMP R0, R1 (E1500001) - 應設置 Z 標誌
    uint32_t inst = 0xE1500001;
    ARM_DataProcessing(core, inst);
    
    ASSERT_TRUE(GBA_CPU_GetFlag_Z(&core->cpu), "Z 標誌應設置（相等）");
    ASSERT_FALSE(GBA_CPU_GetFlag_N(&core->cpu), "N 標誌應清除（非負）");
    
    // CMP R1, R0 (E1510000) - R1 < R0，應設置 N 標誌
    core->cpu.regs.r[1] = 5;
    inst = 0xE1510000;
    ARM_DataProcessing(core, inst);
    
    ASSERT_TRUE(GBA_CPU_GetFlag_N(&core->cpu), "N 標誌應設置（負數）");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_AND() {
    TEST_START("ARM AND");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.r[0] = 0xFF00FF00;
    core->cpu.regs.r[1] = 0xF0F0F0F0;
    
    // AND R2, R0, R1 (E0002001)
    uint32_t inst = 0xE0002001;
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[2], 0xF000F000, "位元 AND 操作");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_ORR() {
    TEST_START("ARM ORR");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.r[0] = 0x0F0F0F0F;
    core->cpu.regs.r[1] = 0xF0F0F0F0;
    
    // ORR R2, R0, R1 (E180200)
    uint32_t inst = 0xE1802001;
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[2], 0xFFFFFFFF, "位元 OR 操作");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STR_LDR_Immediate() {
    TEST_START("ARM STR/LDR 立即數偏移");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[0] = 0x12345678;

    // STR R0, [R1, #4] (E5810004)
    uint32_t str_inst = 0xE5810004;
    ARM_SingleDataTransfer(core, str_inst);

    // LDR R2, [R1, #4] (E5912004)
    uint32_t ldr_inst = 0xE5912004;
    ARM_SingleDataTransfer(core, ldr_inst);

    ASSERT_EQ(core->cpu.regs.r[2], 0x12345678, "LDR 應讀回寫入的值");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDR_STR_MemoryBoundaries() {
    TEST_START("ARM LDR/STR 邊界區域 (BIOS/IO/OAM/VRAM/SRAM)");
    GBA_Core *core = CreateTestCore();

    // BIOS 讀寫邊界
    core->cpu.regs.pc = 0x00000000;
    core->memory.bios[0] = 0x11;
    core->memory.bios[1] = 0x22;
    core->memory.bios[2] = 0x33;
    core->memory.bios[3] = 0x44;
    core->cpu.regs.r[1] = 0x00000000;

    ARM_SingleDataTransfer(core, 0xE5910000); // LDR R0, [R1, #0]
    ASSERT_EQ(core->cpu.regs.r[0], 0x44332211, "BIOS LDR 應讀回資料");

    core->cpu.regs.r[0] = 0xAABBCCDD;
    ARM_SingleDataTransfer(core, 0xE5810000); // STR R0, [R1, #0]
    ASSERT_EQ(core->memory.bios[0], 0x11, "BIOS 應忽略寫入");
    ASSERT_EQ(core->memory.bios[1], 0x22, "BIOS 應忽略寫入");
    ASSERT_EQ(core->memory.bios[2], 0x33, "BIOS 應忽略寫入");
    ASSERT_EQ(core->memory.bios[3], 0x44, "BIOS 應忽略寫入");

    // IO 寄存器寫入
    core->cpu.regs.pc = 0x03000000;
    core->cpu.regs.r[1] = 0x04000200; // IE
    core->cpu.regs.r[0] = 0x00000001;
    core->interrupt.IF = 0;
    ARM_SingleDataTransfer(core, 0xE5810000); // STR R0, [R1, #0]
    ASSERT_EQ(core->interrupt.IE, 0x0001, "IO IE 應被寫入");
    ARM_SingleDataTransfer(core, 0xE5912000); // LDR R2, [R1, #0]
    ASSERT_EQ(core->cpu.regs.r[2], 0x00000001, "IO LDR 應讀回資料");

    // VRAM/OAM 8 位寫入應忽略
    core->cpu.regs.r[1] = 0x06000000; // VRAM
    core->cpu.regs.r[0] = 0x000000FF;
    GBA_MemoryWrite16(core, 0x06000000, 0x1234);
    ARM_SingleDataTransfer(core, 0xE5C10000); // STRB R0, [R1, #0]
    ASSERT_EQ(GBA_MemoryRead16(core, 0x06000000), 0x1234, "VRAM 應忽略 8 位寫入");

    core->cpu.regs.r[1] = 0x07000000; // OAM
    GBA_MemoryWrite16(core, 0x07000000, 0x5678);
    ARM_SingleDataTransfer(core, 0xE5C10000); // STRB R0, [R1, #0]
    ASSERT_EQ(GBA_MemoryRead16(core, 0x07000000), 0x5678, "OAM 應忽略 8 位寫入");

    // SRAM 32 位讀取應複製 8 位
    core->cpu.regs.r[1] = 0x0E000000; // SRAM
    core->cpu.regs.r[0] = 0x0000007F;
    ARM_SingleDataTransfer(core, 0xE5C10000); // STRB R0, [R1, #0]
    ARM_SingleDataTransfer(core, 0xE5913000); // LDR R3, [R1, #0]
    ASSERT_EQ(core->cpu.regs.r[3], 0x7F7F7F7F, "SRAM LDR 應複製 8 位");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_IORegister_SideEffects() {
    TEST_START("ARM IO 特殊寄存器副作用 (IF/IME/WAITCNT)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.pc = 0x03000000;

    // IF 寫 1 清除
    core->interrupt.IF = 0x00FF;
    GBA_MemoryWrite16(core, 0x04000202, 0x0003);
    ASSERT_EQ(core->interrupt.IF, 0x00FC, "IF 寫入應清除對應位");

    // IME 設定
    GBA_MemoryWrite16(core, 0x04000208, 0x0001);
    ASSERT_EQ(core->interrupt.IME, 1, "IME 應設為 1");
    GBA_MemoryWrite16(core, 0x04000208, 0x0000);
    ASSERT_EQ(core->interrupt.IME, 0, "IME 應設為 0");

    // WAITCNT 等待狀態更新
    GBA_MemoryWrite16(core, 0x04000204, 0x04FB);
    ASSERT_EQ(core->memory.wait_states.sram, 4, "WAITCNT SRAM 等待狀態");
    ASSERT_EQ(core->memory.wait_states.ws0_n, 3, "WAITCNT WS0_N 等待狀態");
    ASSERT_EQ(core->memory.wait_states.ws0_s, 1, "WAITCNT WS0_S 等待狀態");
    ASSERT_EQ(core->memory.wait_states.ws1_n, 4, "WAITCNT WS1_N 等待狀態");
    ASSERT_EQ(core->memory.wait_states.ws1_s, 1, "WAITCNT WS1_S 等待狀態");
    ASSERT_EQ(core->memory.wait_states.ws2_n, 1, "WAITCNT WS2_N 等待狀態");
    ASSERT_EQ(core->memory.wait_states.ws2_s, 1, "WAITCNT WS2_S 等待狀態");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_VRAM_OAM_Mirror_Align() {
    TEST_START("VRAM/OAM 鏡像與對齊");
    GBA_Core *core = CreateTestCore();

    // VRAM 鏡像: 0x06018000 -> 0x06010000
    GBA_MemoryWrite32(core, 0x06010000, 0xA1B2C3D4);
    ASSERT_EQ(GBA_MemoryRead32(core, 0x06018000), 0xA1B2C3D4, "VRAM 鏡像讀取應一致");

    // OAM 32 位對齊讀取
    GBA_MemoryWrite32(core, 0x07000000, 0x11223344);
    ASSERT_EQ(GBA_MemoryRead32(core, 0x07000002), 0x11223344, "OAM 32 位未對齊應對齊讀取");

    // OAM 16 位對齊讀取
    GBA_MemoryWrite16(core, 0x07000000, 0xABCD);
    ASSERT_EQ(GBA_MemoryRead16(core, 0x07000001), 0xABCD, "OAM 16 位未對齊應對齊讀取");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDR_UnalignedRotate_Word() {
    TEST_START("ARM LDR 未對齊旋轉 (Word)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000000;
    GBA_MemoryWrite32(core, 0x03000000, 0x11223344);

    // LDR R0, [R1, #1] (E5910001)
    ARM_SingleDataTransfer(core, 0xE5910001);
    ASSERT_EQ(core->cpu.regs.r[0], 0x44112233, "位址+1 應旋轉 8 位");

    // LDR R2, [R1, #2] (E5912002)
    ARM_SingleDataTransfer(core, 0xE5912002);
    ASSERT_EQ(core->cpu.regs.r[2], 0x33441122, "位址+2 應旋轉 16 位");

    // LDR R3, [R1, #3] (E5913003)
    ARM_SingleDataTransfer(core, 0xE5913003);
    ASSERT_EQ(core->cpu.regs.r[3], 0x22334411, "位址+3 應旋轉 24 位");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Return_CPSR_Flags_IF() {
    TEST_START("IRQ 返回 CPSR I/F 與旗標細節");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = (uint32_t)(CPU_STATE_SYSTEM | (1 << 5) | (1 << 7) | (1 << 6) | (1U << 31) | (1U << 29));
    core->cpu.regs.r[14] = 0x0800C003;

    uint32_t inst = 0xE25EF004; // SUBS PC, LR, #4
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (uint32_t)(CPU_STATE_SYSTEM | (1 << 5) | (1 << 7) | (1 << 6) | (1U << 31) | (1U << 29)), "CPSR 應恢復 I/F 與旗標");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STR_UnalignedAlign_Word() {
    TEST_START("ARM STR 未對齊對齊回寫 (Word)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[0] = 0xAABBCCDD;

    // STR R0, [R1, #1] (E5810001)
    ARM_SingleDataTransfer(core, 0xE5810001);

    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000000), 0xAABBCCDD, "STR 應對齊到 4 位址寫入");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STRB_LDRB() {
    TEST_START("ARM STRB/LDRB");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000010;
    core->cpu.regs.r[0] = 0xAB;

    // STRB R0, [R1, #1] (E5C10001)
    uint32_t strb_inst = 0xE5C10001;
    ARM_SingleDataTransfer(core, strb_inst);

    // LDRB R2, [R1, #1] (E5D12001)
    uint32_t ldrb_inst = 0xE5D12001;
    ARM_SingleDataTransfer(core, ldrb_inst);

    ASSERT_EQ(core->cpu.regs.r[2], 0xAB, "LDRB 應讀回 8 位數據");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STR_PostIndex_Writeback() {
    TEST_START("ARM STR 後索引自動回寫");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000020;
    core->cpu.regs.r[0] = 0xCAFEBABE;

    // STR R0, [R1], #4 (E4810004)
    uint32_t inst = 0xE4810004;
    ARM_SingleDataTransfer(core, inst);

    ASSERT_EQ(core->cpu.regs.r[1], 0x03000024, "R1 應自動回寫 +4");

    // LDR R2, [R1, #-4] (E5112004) - 回讀原位置
    uint32_t ldr_inst = 0xE5112004;
    ARM_SingleDataTransfer(core, ldr_inst);

    ASSERT_EQ(core->cpu.regs.r[2], 0xCAFEBABE, "應讀回原位置的值");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_MUL_MLA() {
    TEST_START("ARM MUL/MLA");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[0] = 6;
    core->cpu.regs.r[1] = 7;
    core->cpu.regs.r[2] = 5;

    // MUL R3, R0, R1 (E0030190)
    uint32_t mul_inst = 0xE0030190;
    ARM_Multiply(core, mul_inst);
    ASSERT_EQ(core->cpu.regs.r[3], 42, "6 * 7 = 42");

    // MLA R4, R0, R1, R2 (E0242190)
    uint32_t mla_inst = 0xE0242190;
    ARM_Multiply(core, mla_inst);
    ASSERT_EQ(core->cpu.regs.r[4], 47, "6 * 7 + 5 = 47");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_MULLong() {
    TEST_START("ARM UMULL/UMLAL/SMULL");
    GBA_Core *core = CreateTestCore();

    // UMULL R1:R0 = R2 * R3
    core->cpu.regs.r[2] = 0x10000000;
    core->cpu.regs.r[3] = 0x10;
    uint32_t umull_inst = 0xE0810392;  // UMULL R1,R0,R2,R3
    ARM_MultiplyLong(core, umull_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0x00000000, "UMULL 低位應為 0");
    ASSERT_EQ(core->cpu.regs.r[1], 0x00000001, "UMULL 高位應為 1");

    // UMLAL R1:R0 += R2 * R3
    core->cpu.regs.r[0] = 0x00000005;
    core->cpu.regs.r[1] = 0x00000001;
    uint32_t umlal_inst = 0xE0A10392;  // UMLAL R1,R0,R2,R3
    ARM_MultiplyLong(core, umlal_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0x00000005, "UMLAL 低位應保留累加");
    ASSERT_EQ(core->cpu.regs.r[1], 0x00000002, "UMLAL 高位應加 1");

    // SMULL R5:R4 = R2 * R3 (signed)
    core->cpu.regs.r[2] = 0xFFFFFFFE;  // -2
    core->cpu.regs.r[3] = 0x00000003;  // 3
    uint32_t smull_inst = 0xE0C54392;  // SMULL R5,R4,R2,R3
    ARM_MultiplyLong(core, smull_inst);
    ASSERT_EQ(core->cpu.regs.r[4], 0xFFFFFFFA, "SMULL 低位應為 -6");
    ASSERT_EQ(core->cpu.regs.r[5], 0xFFFFFFFF, "SMULL 高位應為符號擴展");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_STM() {
    TEST_START("ARM LDM/STM");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[0] = 0x11111111;
    core->cpu.regs.r[1] = 0x22222222;
    core->cpu.regs.r[2] = 0x33333333;
    core->cpu.regs.r[3] = 0x03000000;

    // STMIA R3!, {R0-R2} (E8A30007)
    uint32_t stm_inst = 0xE8A30007;
    ARM_BlockDataTransfer(core, stm_inst);

    ASSERT_EQ(core->cpu.regs.r[3], 0x0300000C, "R3 應寫回 +12");

    // 清空暫存器並重置 base
    core->cpu.regs.r[0] = 0;
    core->cpu.regs.r[1] = 0;
    core->cpu.regs.r[2] = 0;
    core->cpu.regs.r[3] = 0x03000000;

    // LDMIA R3, {R0-R2} (E8930007) - 不寫回
    uint32_t ldm_inst = 0xE8930007;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0x11111111, "LDM 應載入 R0");
    ASSERT_EQ(core->cpu.regs.r[1], 0x22222222, "LDM 應載入 R1");
    ASSERT_EQ(core->cpu.regs.r[2], 0x33333333, "LDM 應載入 R2");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_STM_Decrement() {
    TEST_START("ARM LDM/STM 遞減模式");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[0] = 0xAAAAAAAA;
    core->cpu.regs.r[1] = 0xBBBBBBBB;
    core->cpu.regs.r[3] = 0x03000020;

    // STMDB R3!, {R0,R1} (E9230003)
    uint32_t stmdb_inst = 0xE9230003;
    ARM_BlockDataTransfer(core, stmdb_inst);
    ASSERT_EQ(core->cpu.regs.r[3], 0x03000018, "R3 應回寫 -8");

    core->cpu.regs.r[0] = 0;
    core->cpu.regs.r[1] = 0;

    // LDMIA R3!, {R0,R1} (E8B30003)
    uint32_t ldmia_inst = 0xE8B30003;
    ARM_BlockDataTransfer(core, ldmia_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0xAAAAAAAA, "LDM 應載入 R0");
    ASSERT_EQ(core->cpu.regs.r[1], 0xBBBBBBBB, "LDM 應載入 R1");
    ASSERT_EQ(core->cpu.regs.r[3], 0x03000020, "R3 應回寫 +8");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_PC_Writeback() {
    TEST_START("ARM LDM 含 PC");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000000;
    GBA_MemoryWrite32(core, 0x03000000, 0x11111111);
    GBA_MemoryWrite32(core, 0x03000004, 0x08000009);

    // LDMIA R1!, {R0, PC} (E8B18001)
    uint32_t ldm_inst = 0xE8B18001;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0x11111111, "LDM 應載入 R0");
    ASSERT_EQ(core->cpu.regs.pc, 0x08000008, "PC 應對齊並載入");
    ASSERT_EQ(core->cpu.regs.r[1], 0x03000008, "R1 應回寫 +8");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_RnInList_NoWriteback() {
    TEST_START("ARM LDM Rn 在清單內");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[4] = 0x03000010;
    GBA_MemoryWrite32(core, 0x03000010, 0xCAFEBABE);
    GBA_MemoryWrite32(core, 0x03000014, 0xDEADBEEF);

    // LDMIA R4!, {R0, R4} (E8B40011)
    uint32_t ldm_inst = 0xE8B40011;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0xCAFEBABE, "LDM 應載入 R0");
    ASSERT_EQ(core->cpu.regs.r[4], 0xDEADBEEF, "R4 應載入清單內值");
    ASSERT_FALSE(core->cpu.regs.r[4] == 0x03000018, "Rn 在清單內時不應回寫");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STM_PC_Writeback() {
    TEST_START("ARM STM 含 PC");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[3] = 0x03000020;
    core->cpu.regs.r[0] = 0x12345678;
    core->cpu.regs.pc = 0x08001234;

    // STMIA R3!, {R0, PC} (E8A38001)
    uint32_t stm_inst = 0xE8A38001;
    ARM_BlockDataTransfer(core, stm_inst);

    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000020), 0x12345678, "STM 應存 R0");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000024), 0x08001234, "STM 應存 PC");
    ASSERT_EQ(core->cpu.regs.r[3], 0x03000028, "R3 應回寫 +8");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_RestoreCPSR() {
    TEST_START("ARM LDM^ 含 PC 恢復 CPSR");
    GBA_Core *core = CreateTestCore();

    // 模擬 Supervisor 模式，SPSR 指向 THUMB + User 模式
    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR;
    core->cpu.regs.svc.spsr = CPU_STATE_USER | (1 << 5);

    core->cpu.regs.r[1] = 0x03000000;
    GBA_MemoryWrite32(core, 0x03000000, 0x08000009);

    // LDMIA R1!, {PC}^ (E8F18000)
    uint32_t ldm_inst = 0xE8F18000;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.cpsr, (CPU_STATE_USER | (1 << 5)), "CPSR 應恢復為 SPSR");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切換到 THUMB 模式");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STM_S_UserBank() {
    TEST_START("ARM STM^ 使用 User bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[0] = 0x11111111;
    core->cpu.regs.r[13] = 0x03007FE0;  // SVC SP
    core->cpu.regs.usr.r13_usr = 0x03007F00;  // User SP

    // STMIA R1, {R0, R13}^ (E8C12001)
    uint32_t stm_inst = 0xE8C12001;
    ARM_BlockDataTransfer(core, stm_inst);

    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000000), 0x11111111, "應存 R0");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000004), 0x03007F00, "應存 User R13");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_UserBank() {
    TEST_START("ARM LDM^ 載入 User bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x03000010;
    core->cpu.regs.r[13] = 0x03007FE0;  // SVC SP
    core->cpu.regs.usr.r13_usr = 0x03007F00;  // User SP

    GBA_MemoryWrite32(core, 0x03000010, 0x22222222);
    GBA_MemoryWrite32(core, 0x03000014, 0x0000ABCD);

    // LDMIA R1, {R0, R13}^ (E8D12001)
    uint32_t ldm_inst = 0xE8D12001;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0x22222222, "R0 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r13_usr, 0x0000ABCD, "User R13 應更新");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC R13 不應改變");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STM_S_UserBank_FIQ_R8_R12() {
    TEST_START("ARM STM^ FIQ R8-R12 使用 User bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_FIQ;
    core->cpu.regs.r[1] = 0x03000030;

    // FIQ banked registers
    core->cpu.regs.r[8] = 0x11111111;
    core->cpu.regs.r[9] = 0x22222222;
    core->cpu.regs.r[10] = 0x33333333;
    core->cpu.regs.r[11] = 0x44444444;
    core->cpu.regs.r[12] = 0x55555555;

    // User bank registers
    core->cpu.regs.usr.r8_usr = 0xAAAAAAA1;
    core->cpu.regs.usr.r9_usr = 0xAAAAAAA2;
    core->cpu.regs.usr.r10_usr = 0xAAAAAAA3;
    core->cpu.regs.usr.r11_usr = 0xAAAAAAA4;
    core->cpu.regs.usr.r12_usr = 0xAAAAAAA5;

    // STMIA R1, {R8-R12}^ (E8C11F00)
    uint32_t stm_inst = 0xE8C11F00;
    ARM_BlockDataTransfer(core, stm_inst);

    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000030), 0xAAAAAAA1, "應存 User R8");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000034), 0xAAAAAAA2, "應存 User R9");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000038), 0xAAAAAAA3, "應存 User R10");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x0300003C), 0xAAAAAAA4, "應存 User R11");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000040), 0xAAAAAAA5, "應存 User R12");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_UserBank_FIQ_R8_R12() {
    TEST_START("ARM LDM^ FIQ R8-R12 載入 User bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_FIQ;
    core->cpu.regs.r[1] = 0x03000050;

    core->cpu.regs.r[8] = 0x11111111;
    core->cpu.regs.r[9] = 0x22222222;
    core->cpu.regs.r[10] = 0x33333333;
    core->cpu.regs.r[11] = 0x44444444;
    core->cpu.regs.r[12] = 0x55555555;

    GBA_MemoryWrite32(core, 0x03000050, 0xBBBBBBB1);
    GBA_MemoryWrite32(core, 0x03000054, 0xBBBBBBB2);
    GBA_MemoryWrite32(core, 0x03000058, 0xBBBBBBB3);
    GBA_MemoryWrite32(core, 0x0300005C, 0xBBBBBBB4);
    GBA_MemoryWrite32(core, 0x03000060, 0xBBBBBBB5);

    // LDMIA R1, {R8-R12}^ (E8D11F00)
    uint32_t ldm_inst = 0xE8D11F00;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.usr.r8_usr, 0xBBBBBBB1, "User R8 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r9_usr, 0xBBBBBBB2, "User R9 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r10_usr, 0xBBBBBBB3, "User R10 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r11_usr, 0xBBBBBBB4, "User R11 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r12_usr, 0xBBBBBBB5, "User R12 應更新");

    ASSERT_EQ(core->cpu.regs.r[8], 0x11111111, "FIQ R8 不應改變");
    ASSERT_EQ(core->cpu.regs.r[9], 0x22222222, "FIQ R9 不應改變");
    ASSERT_EQ(core->cpu.regs.r[10], 0x33333333, "FIQ R10 不應改變");
    ASSERT_EQ(core->cpu.regs.r[11], 0x44444444, "FIQ R11 不應改變");
    ASSERT_EQ(core->cpu.regs.r[12], 0x55555555, "FIQ R12 不應改變");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STM_S_UserBank_IRQ_SVC() {
    TEST_START("ARM STM^ IRQ/SVC User bank R13/R14");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.r[1] = 0x03000070;
    core->cpu.regs.r[13] = 0x03007FA0;  // IRQ SP
    core->cpu.regs.r[14] = 0x08000010;  // IRQ LR
    core->cpu.regs.usr.r13_usr = 0x03007F00;
    core->cpu.regs.usr.r14_usr = 0x08000004;

    // STMIA R1, {R13,R14}^ (E8C16000)
    uint32_t stm_inst = 0xE8C16000;
    ARM_BlockDataTransfer(core, stm_inst);
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000070), 0x03007F00, "IRQ 應存 User R13");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000074), 0x08000004, "IRQ 應存 User R14");

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x03000090;
    core->cpu.regs.r[13] = 0x03007FE0;  // SVC SP
    core->cpu.regs.r[14] = 0x08000020;  // SVC LR
    core->cpu.regs.usr.r13_usr = 0x03007F10;
    core->cpu.regs.usr.r14_usr = 0x08000008;

    ARM_BlockDataTransfer(core, stm_inst);
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000090), 0x03007F10, "SVC 應存 User R13");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000094), 0x08000008, "SVC 應存 User R14");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_UserBank_IRQ_SVC() {
    TEST_START("ARM LDM^ IRQ/SVC User bank R13/R14");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.r[1] = 0x030000B0;
    core->cpu.regs.r[13] = 0x03007FA0;
    core->cpu.regs.r[14] = 0x08000010;
    core->cpu.regs.usr.r13_usr = 0x03007F00;
    core->cpu.regs.usr.r14_usr = 0x08000004;

    GBA_MemoryWrite32(core, 0x030000B0, 0x03007F20);
    GBA_MemoryWrite32(core, 0x030000B4, 0x0800000C);

    // LDMIA R1, {R13,R14}^ (E8D16000)
    uint32_t ldm_inst = 0xE8D16000;
    ARM_BlockDataTransfer(core, ldm_inst);
    ASSERT_EQ(core->cpu.regs.usr.r13_usr, 0x03007F20, "IRQ User R13 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r14_usr, 0x0800000C, "IRQ User R14 應更新");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FA0, "IRQ R13 不應改變");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000010, "IRQ R14 不應改變");

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x030000D0;
    core->cpu.regs.r[13] = 0x03007FE0;
    core->cpu.regs.r[14] = 0x08000020;
    core->cpu.regs.usr.r13_usr = 0x03007F10;
    core->cpu.regs.usr.r14_usr = 0x08000008;

    GBA_MemoryWrite32(core, 0x030000D0, 0x03007F30);
    GBA_MemoryWrite32(core, 0x030000D4, 0x08000014);

    ARM_BlockDataTransfer(core, ldm_inst);
    ASSERT_EQ(core->cpu.regs.usr.r13_usr, 0x03007F30, "SVC User R13 應更新");
    ASSERT_EQ(core->cpu.regs.usr.r14_usr, 0x08000014, "SVC User R14 應更新");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC R13 不應改變");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000020, "SVC R14 不應改變");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_PC_RnInList_NoWriteback() {
    TEST_START("ARM LDM^ 含 PC+Rn 不回寫");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SYSTEM | (1 << 5);

    core->cpu.regs.r[1] = 0x03000140;
    GBA_MemoryWrite32(core, 0x03000140, 0x11111111);
    GBA_MemoryWrite32(core, 0x03000144, 0x08001003);

    // LDMIA R1!, {R1, PC}^ (E8F18002)
    uint32_t ldm_inst = 0xE8F18002;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[1], 0x11111111, "R1 應載入清單值");
    ASSERT_EQ(core->cpu.regs.pc, 0x08001000, "PC 應對齊並載入");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_PC_NoUserBankLoad() {
    TEST_START("ARM LDM^ 含 PC 不載入 User bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR | (1 << 7);
    core->cpu.regs.svc.spsr = CPU_STATE_SUPERVISOR;

    core->cpu.regs.r[1] = 0x03000160;
    core->cpu.regs.r[13] = 0x03007FE0;
    core->cpu.regs.usr.r13_usr = 0x03007F00;

    GBA_MemoryWrite32(core, 0x03000160, 0x03007F88);
    GBA_MemoryWrite32(core, 0x03000164, 0x08002003);

    // LDMIA R1, {R13, PC}^ (E8D1A000)
    uint32_t ldm_inst = 0xE8D1A000;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F88, "R13 應更新為載入值");
    ASSERT_EQ(core->cpu.regs.usr.r13_usr, 0x03007F00, "User R13 不應更新");
    ASSERT_EQ(core->cpu.regs.pc, 0x08002000, "PC 應對齊並載入");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_PC_PipelineFlush() {
    TEST_START("ARM LDM PC 觸發 pipeline flush");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.r[1] = 0x03000180;
    GBA_MemoryWrite32(core, 0x03000180, 0x08003001);

    // LDMIA R1!, {PC} (E8B18000)
    uint32_t ldm_inst = 0xE8B18000;
    InstructionResult result = ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_TRUE(result.branch_taken, "LDM PC 應視為分支");
    ASSERT_TRUE(result.pipeline_flush, "LDM PC 應刷新流水線");
    ASSERT_EQ(core->cpu.regs.pc, 0x08003000, "PC 應對齊並載入");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_PC_SwitchBank_SVC() {
    TEST_START("ARM LDM^ PC 切換到 SVC bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x030001A0;

    core->cpu.regs.svc.r13 = 0x03007FE0;
    core->cpu.regs.svc.r14 = 0x08000060;
    core->cpu.regs.r[13] = 0x03007FA0;
    core->cpu.regs.r[14] = 0x08000010;

    GBA_MemoryWrite32(core, 0x030001A0, 0x08004003);

    // LDMIA R1!, {PC}^ (E8F18000)
    uint32_t ldm_inst = 0xE8F18000;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SUPERVISOR, "應回到 SVC 模式");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000060, "SVC LR 應恢復 bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_PC_SwitchBank_FIQ() {
    TEST_START("ARM LDM^ PC 切換到 FIQ bank");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_FIQ;
    core->cpu.regs.r[1] = 0x030001C0;

    core->cpu.regs.fiq.r8_fiq = 0xF8000028;
    core->cpu.regs.fiq.r9_fiq = 0xF9000029;
    core->cpu.regs.fiq.r10_fiq = 0xFA00002A;
    core->cpu.regs.fiq.r11_fiq = 0xFB00002B;
    core->cpu.regs.fiq.r12_fiq = 0xFC00002C;
    core->cpu.regs.fiq.r13_fiq = 0x03007FF4;
    core->cpu.regs.fiq.r14_fiq = 0x08000070;

    GBA_MemoryWrite32(core, 0x030001C0, 0x08005003);

    // LDMIA R1!, {PC}^ (E8F18000)
    uint32_t ldm_inst = 0xE8F18000;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_FIQ, "應回到 FIQ 模式");
    ASSERT_EQ(core->cpu.regs.r[8], 0xF8000028, "FIQ R8 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[12], 0xFC00002C, "FIQ R12 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FF4, "FIQ SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000070, "FIQ LR 應恢復 bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_STM_S_Writeback_RnInList() {
    TEST_START("ARM STM^ Rn 在清單內 + 回寫");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x03000100;
    core->cpu.regs.r[13] = 0x03007FE0;
    core->cpu.regs.usr.r13_usr = 0x03007F10;

    // STMIA R1!, {R1, R13}^ (E8E12002)
    uint32_t stm_inst = 0xE8E12002;
    ARM_BlockDataTransfer(core, stm_inst);

    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000100), 0x03000100, "應存 R1 本身");
    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000104), 0x03007F10, "應存 User R13");
    ASSERT_EQ(core->cpu.regs.r[1], 0x03000108, "Rn 在清單內仍應回寫");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_LDM_S_RnInList_NoWriteback() {
    TEST_START("ARM LDM^ Rn 在清單內不回寫");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[1] = 0x03000120;
    core->cpu.regs.r[13] = 0x03007FE0;
    core->cpu.regs.usr.r13_usr = 0x03007F00;

    GBA_MemoryWrite32(core, 0x03000120, 0x11111111);
    GBA_MemoryWrite32(core, 0x03000124, 0x03007F30);

    // LDMIA R1!, {R1, R13}^ (E8F12002)
    uint32_t ldm_inst = 0xE8F12002;
    ARM_BlockDataTransfer(core, ldm_inst);

    ASSERT_EQ(core->cpu.regs.r[1], 0x11111111, "R1 應載入清單值");
    ASSERT_EQ(core->cpu.regs.usr.r13_usr, 0x03007F30, "User R13 應更新");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC R13 不應改變");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_CPU_ModeSwitch_IRQ_SVC_FIQ() {
    TEST_START("CPU 模式切換 IRQ/SVC/FIQ");
    GBA_Core *core = CreateTestCore();

    // 初始 User/System 寄存器
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.r[8] = 0x100;
    core->cpu.regs.r[9] = 0x101;
    core->cpu.regs.r[10] = 0x102;
    core->cpu.regs.r[11] = 0x103;
    core->cpu.regs.r[12] = 0x104;
    core->cpu.regs.r[13] = 0x03007F00;
    core->cpu.regs.r[14] = 0x08000000;

    // IRQ 模式寄存器
    GBA_CPU_SwitchMode(core, CPU_STATE_IRQ);
    core->cpu.regs.r[13] = 0x03007FA0;
    core->cpu.regs.r[14] = 0x08000010;

    // SVC 模式寄存器
    GBA_CPU_SwitchMode(core, CPU_STATE_SUPERVISOR);
    core->cpu.regs.r[13] = 0x03007FE0;
    core->cpu.regs.r[14] = 0x08000020;

    // FIQ 模式寄存器 (含 r8-r12)
    GBA_CPU_SwitchMode(core, CPU_STATE_FIQ);
    core->cpu.regs.r[8] = 0x200;
    core->cpu.regs.r[9] = 0x201;
    core->cpu.regs.r[10] = 0x202;
    core->cpu.regs.r[11] = 0x203;
    core->cpu.regs.r[12] = 0x204;
    core->cpu.regs.r[13] = 0x03007FF0;
    core->cpu.regs.r[14] = 0x08000030;

    // 切回 System 應還原 User/System 寄存器
    GBA_CPU_SwitchMode(core, CPU_STATE_SYSTEM);
    ASSERT_EQ(core->cpu.regs.r[8], 0x100, "System r8 應還原");
    ASSERT_EQ(core->cpu.regs.r[12], 0x104, "System r12 應還原");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F00, "System SP 應還原");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000000, "System LR 應還原");

    // 切回 IRQ 應還原 IRQ 寄存器
    GBA_CPU_SwitchMode(core, CPU_STATE_IRQ);
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FA0, "IRQ SP 應還原");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000010, "IRQ LR 應還原");

    // 切回 SVC 應還原 SVC 寄存器
    GBA_CPU_SwitchMode(core, CPU_STATE_SUPERVISOR);
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC SP 應還原");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000020, "SVC LR 應還原");

    // 切回 FIQ 應還原 FIQ 寄存器
    GBA_CPU_SwitchMode(core, CPU_STATE_FIQ);
    ASSERT_EQ(core->cpu.regs.r[8], 0x200, "FIQ r8 應還原");
    ASSERT_EQ(core->cpu.regs.r[12], 0x204, "FIQ r12 應還原");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FF0, "FIQ SP 應還原");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000030, "FIQ LR 應還原");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_CPU_ModeSwitch_FIQ_System_RoundTrip() {
    TEST_START("FIQ <-> System bank 往返");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.r[8] = 0x11000008;
    core->cpu.regs.r[9] = 0x11000009;
    core->cpu.regs.r[10] = 0x1100000A;
    core->cpu.regs.r[11] = 0x1100000B;
    core->cpu.regs.r[12] = 0x1100000C;
    core->cpu.regs.r[13] = 0x03007F00;
    core->cpu.regs.r[14] = 0x08000000;

    core->cpu.regs.usr.r8_usr = 0x22000008;
    core->cpu.regs.usr.r9_usr = 0x22000009;
    core->cpu.regs.usr.r10_usr = 0x2200000A;
    core->cpu.regs.usr.r11_usr = 0x2200000B;
    core->cpu.regs.usr.r12_usr = 0x2200000C;
    core->cpu.regs.usr.r13_usr = 0x03007F10;
    core->cpu.regs.usr.r14_usr = 0x08000010;

    GBA_CPU_SwitchMode(core, CPU_STATE_FIQ);
    core->cpu.regs.r[8] = 0xF8000008;
    core->cpu.regs.r[9] = 0xF9000009;
    core->cpu.regs.r[10] = 0xFA00000A;
    core->cpu.regs.r[11] = 0xFB00000B;
    core->cpu.regs.r[12] = 0xFC00000C;
    core->cpu.regs.r[13] = 0x03007FF0;
    core->cpu.regs.r[14] = 0x08000030;

    GBA_CPU_SwitchMode(core, CPU_STATE_SYSTEM);
    ASSERT_EQ(core->cpu.regs.r[8], 0x11000008, "System R8 應還原");
    ASSERT_EQ(core->cpu.regs.r[9], 0x11000009, "System R9 應還原");
    ASSERT_EQ(core->cpu.regs.r[10], 0x1100000A, "System R10 應還原");
    ASSERT_EQ(core->cpu.regs.r[11], 0x1100000B, "System R11 應還原");
    ASSERT_EQ(core->cpu.regs.r[12], 0x1100000C, "System R12 應還原");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F00, "System SP 應還原");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000000, "System LR 應還原");

    GBA_CPU_SwitchMode(core, CPU_STATE_FIQ);
    ASSERT_EQ(core->cpu.regs.r[8], 0xF8000008, "FIQ R8 應還原");
    ASSERT_EQ(core->cpu.regs.r[9], 0xF9000009, "FIQ R9 應還原");
    ASSERT_EQ(core->cpu.regs.r[10], 0xFA00000A, "FIQ R10 應還原");
    ASSERT_EQ(core->cpu.regs.r[11], 0xFB00000B, "FIQ R11 應還原");
    ASSERT_EQ(core->cpu.regs.r[12], 0xFC00000C, "FIQ R12 應還原");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FF0, "FIQ SP 應還原");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000030, "FIQ LR 應還原");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_FIQ_From_THUMB() {
    TEST_START("FIQ 觸發流程 (THUMB)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 5);
    core->cpu.regs.pc = 0x08001000;
    core->cpu.state.fiq_line = true;

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_FIQ, "應切換到 FIQ 模式");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "FIQ 應切回 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x0000001C, "PC 應跳到 FIQ 向量");
    ASSERT_EQ(core->cpu.regs.fiq.r14_fiq, 0x08001000, "LR 應保存 THUMB 返回地址");
    ASSERT_EQ(core->cpu.regs.fiq.spsr_fiq, (CPU_STATE_SYSTEM | (1 << 5)), "SPSR 應保存原 CPSR");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 6), "應禁用 FIQ");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 7), "應禁用 IRQ");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_FIQ_From_ARM() {
    TEST_START("FIQ 觸發流程 (ARM)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM;
    core->cpu.regs.pc = 0x08001234;
    core->cpu.state.fiq_line = true;

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_FIQ, "應切換到 FIQ 模式");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "FIQ 應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x0000001C, "PC 應跳到 FIQ 向量");
    ASSERT_EQ(core->cpu.regs.fiq.r14_fiq, 0x08001230, "LR 應保存 ARM 返回地址");
    ASSERT_EQ(core->cpu.regs.fiq.spsr_fiq, CPU_STATE_SYSTEM, "SPSR 應保存原 CPSR");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 6), "應禁用 FIQ");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 7), "應禁用 IRQ");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Masked_By_I() {
    TEST_START("IRQ 被 I 旗標遮蔽");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 7);
    core->cpu.regs.pc = 0x08002000;

    core->interrupt.IE = IRQ_VBLANK;
    core->interrupt.IME = 1;
    GBA_InterruptRequest(core, IRQ_VBLANK);

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "I=1 不應進入 IRQ");
    ASSERT_EQ(core->cpu.regs.pc, 0x08002000, "PC 不應改變");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_FIQ_Masked_By_F() {
    TEST_START("FIQ 被 F 旗標遮蔽");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 6);
    core->cpu.regs.pc = 0x08003000;
    core->cpu.state.fiq_line = true;

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "F=1 不應進入 FIQ");
    ASSERT_EQ(core->cpu.regs.pc, 0x08003000, "PC 不應改變");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_From_ARM() {
    TEST_START("IRQ 觸發流程 (ARM)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM;  // 允許 IRQ
    core->cpu.regs.pc = 0x08000100;

    core->interrupt.IE = IRQ_VBLANK;
    core->interrupt.IME = 1;
    GBA_InterruptRequest(core, IRQ_VBLANK);

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_IRQ, "應切換到 IRQ 模式");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 7), "應禁用 IRQ");
    ASSERT_EQ(core->cpu.regs.pc, 0x00000018, "PC 應跳到 IRQ 向量");
    ASSERT_EQ(core->cpu.regs.irq.r14, 0x080000FC, "LR 應保存返回地址");
    ASSERT_EQ(core->cpu.regs.irq.spsr, CPU_STATE_SYSTEM, "SPSR 應保存原 CPSR");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_From_THUMB() {
    TEST_START("IRQ 觸發流程 (THUMB)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 5);  // THUMB 狀態
    core->cpu.regs.pc = 0x08000200;

    core->interrupt.IE = IRQ_TIMER0;
    core->interrupt.IME = 1;
    GBA_InterruptRequest(core, IRQ_TIMER0);

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_IRQ, "應切換到 IRQ 模式");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "IRQ 應切回 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x00000018, "PC 應跳到 IRQ 向量");
    ASSERT_EQ(core->cpu.regs.irq.r14, 0x08000200, "LR 應保存 THUMB 返回地址");
    ASSERT_EQ(core->cpu.regs.irq.spsr, (CPU_STATE_SYSTEM | (1 << 5)), "SPSR 應保存原 CPSR");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Vector_PipelineFlush_ARM_Boundary() {
    TEST_START("IRQ 向量/延遲槽 邊界 (ARM)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM;
    core->cpu.regs.pc = 0x08000004;

    core->memory.bios[0x18] = 0x78;
    core->memory.bios[0x19] = 0x56;
    core->memory.bios[0x1A] = 0x34;
    core->memory.bios[0x1B] = 0x12;

    core->interrupt.IE = IRQ_VBLANK;
    core->interrupt.IME = 1;
    GBA_InterruptRequest(core, IRQ_VBLANK);

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.pc, 0x00000018, "PC 應跳到 IRQ 向量");
    ASSERT_EQ(core->cpu.regs.irq.r14, 0x08000000, "LR 應保存 ARM 返回地址");
    ASSERT_EQ(core->cpu.regs.pipeline[0], 0x12345678, "IRQ 向量應刷新流水線");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Vector_PipelineFlush_THUMB_Boundary() {
    TEST_START("IRQ 向量/延遲槽 邊界 (THUMB)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 5);
    core->cpu.regs.pc = 0x08000002;

    core->memory.bios[0x18] = 0xEF;
    core->memory.bios[0x19] = 0xBE;
    core->memory.bios[0x1A] = 0xAD;
    core->memory.bios[0x1B] = 0xDE;

    core->interrupt.IE = IRQ_TIMER0;
    core->interrupt.IME = 1;
    GBA_InterruptRequest(core, IRQ_TIMER0);

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "IRQ 應切回 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x00000018, "PC 應跳到 IRQ 向量");
    ASSERT_EQ(core->cpu.regs.irq.r14, 0x08000002, "LR 應保存 THUMB 返回地址");
    ASSERT_EQ(core->cpu.regs.pipeline[0], 0xDEADBEEF, "IRQ 向量應刷新流水線");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_FIQ_Priority_Over_IRQ_Vector() {
    TEST_START("FIQ 優先於 IRQ 向量");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM;
    core->cpu.regs.pc = 0x08001000;
    core->cpu.state.fiq_line = true;

    core->memory.bios[0x1C] = 0xAA;
    core->memory.bios[0x1D] = 0xBB;
    core->memory.bios[0x1E] = 0xCC;
    core->memory.bios[0x1F] = 0xDD;

    core->interrupt.IE = IRQ_VBLANK;
    core->interrupt.IME = 1;
    GBA_InterruptRequest(core, IRQ_VBLANK);

    GBA_CPUCheckInterrupts(core);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_FIQ, "FIQ 應先進入");
    ASSERT_EQ(core->cpu.regs.pc, 0x0000001C, "PC 應跳到 FIQ 向量");
    ASSERT_EQ(core->cpu.regs.pipeline[0], 0xDDCCBBAA, "FIQ 向量應刷新流水線");
    ASSERT_FALSE(core->cpu.state.fiq_line, "FIQ 線路應清除");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_SWI_From_ARM() {
    TEST_START("SWI 觸發流程 (ARM)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM;
    core->cpu.regs.pc = 0x08000300;

    // SWI 0x01 (EF000001)
    uint32_t swi_inst = 0xEF000001;
    ARM_SoftwareInterrupt(core, swi_inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SUPERVISOR, "應切換到 SVC 模式");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 7), "應禁用 IRQ");
    ASSERT_EQ(core->cpu.regs.pc, 0x00000008, "PC 應跳到 SWI 向量");
    ASSERT_EQ(core->cpu.regs.svc.r14, 0x080002FC, "LR 應保存返回地址");
    ASSERT_EQ(core->cpu.regs.svc.spsr, CPU_STATE_SYSTEM, "SPSR 應保存原 CPSR");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_SWI_From_THUMB() {
    TEST_START("SWI 觸發流程 (THUMB)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 5);
    core->cpu.regs.pc = 0x08000400;

    // THUMB SWI 0x02 (DF02)
    uint16_t swi_inst = 0xDF02;
    THUMB_SoftwareInterrupt(core, swi_inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SUPERVISOR, "應切換到 SVC 模式");
    ASSERT_TRUE(core->cpu.regs.cpsr & (1 << 7), "應禁用 IRQ");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "SWI 應切回 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x00000008, "PC 應跳到 SWI 向量");
    ASSERT_EQ(core->cpu.regs.svc.r14, 0x080003FE, "LR 應保存返回地址");
    ASSERT_EQ(core->cpu.regs.svc.spsr, (CPU_STATE_SYSTEM | (1 << 5)), "SPSR 應保存原 CPSR");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Return_SUBS_PC_LR() {
    TEST_START("IRQ 返回 SUBS PC, LR, #4");
    GBA_Core *core = CreateTestCore();

    // 模擬 IRQ 內部狀態
    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SYSTEM | (1 << 5);  // 回到 THUMB
    core->cpu.regs.r[14] = 0x08001004;

    // SUBS PC, LR, #4 (E25EF004)
    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08001000, "PC 應回到中斷前");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (CPU_STATE_SYSTEM | (1 << 5)), "CPSR 應恢復");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_SWI_Return_SUBS_PC_LR() {
    TEST_START("SWI 返回 SUBS PC, LR, #4");
    GBA_Core *core = CreateTestCore();

    // 模擬 SVC 內部狀態
    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR | (1 << 7);
    core->cpu.regs.svc.spsr = CPU_STATE_SYSTEM;
    core->cpu.regs.r[14] = 0x08002004;

    // SUBS PC, LR, #4 (E25EF004)
    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08002000, "PC 應回到 SWI 前");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, CPU_STATE_SYSTEM, "CPSR 應恢復");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_SWI_Return_To_THUMB() {
    TEST_START("SWI 返回到 THUMB");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR | (1 << 7);
    core->cpu.regs.svc.spsr = CPU_STATE_SYSTEM | (1 << 5);  // 回到 THUMB
    core->cpu.regs.r[14] = 0x08003003;

    // SUBS PC, LR, #4 (E25EF004)
    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08002FFE, "PC 應回到 THUMB 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (CPU_STATE_SYSTEM | (1 << 5)), "CPSR 應恢復");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Return_To_ARM() {
    TEST_START("IRQ 返回到 ARM");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SYSTEM;  // 回到 ARM
    core->cpu.regs.r[14] = 0x08004004;
    core->cpu.regs.r[13] = 0x03007FA0;
    core->cpu.regs.usr.r13_usr = 0x03007F00;
    core->cpu.regs.usr.r14_usr = 0x08000000;

    // SUBS PC, LR, #4 (E25EF004)
    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08004000, "PC 應回到 ARM 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, CPU_STATE_SYSTEM, "CPSR 應恢復");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F00, "System SP 應恢復 User bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000000, "System LR 應恢復 User bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_SWI_Return_To_ARM_WithFlags() {
    TEST_START("SWI 返回 ARM + CPSR 標誌");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR | (1 << 7);
    core->cpu.regs.svc.spsr = CPU_STATE_SYSTEM | (1 << 31) | (1 << 30);  // N/Z
    core->cpu.regs.r[14] = 0x08005004;
    core->cpu.regs.r[13] = 0x03007FE0;
    core->cpu.regs.usr.r13_usr = 0x03007F00;
    core->cpu.regs.usr.r14_usr = 0x08000000;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08005000, "PC 應回到 ARM 位址");
    ASSERT_EQ(core->cpu.regs.cpsr, (uint32_t)(CPU_STATE_SYSTEM | (1U << 31) | (1U << 30)), "CPSR 應恢復標誌");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F00, "System SP 應恢復 User bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000000, "System LR 應恢復 User bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Return_To_SVC() {
    TEST_START("IRQ 返回到 SVC");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SUPERVISOR;  // 回到 SVC
    core->cpu.regs.r[14] = 0x08006004;
    core->cpu.regs.r[13] = 0x03007FA0;
    core->cpu.regs.svc.r13 = 0x03007FE0;
    core->cpu.regs.svc.r14 = 0x08000020;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08006000, "PC 應回到 SVC 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SUPERVISOR, "應回到 SVC 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, CPU_STATE_SUPERVISOR, "CPSR 應恢復");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000020, "SVC LR 應恢復 bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ABT_Return_SUBS_PC_LR() {
    TEST_START("ABT 返回 SUBS PC, LR, #4");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_ABORT;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_ABORT | (1 << 7);
    core->cpu.regs.abt.spsr = CPU_STATE_SYSTEM | (1 << 5);
    core->cpu.regs.r[14] = 0x0800A003;
    core->cpu.regs.r[13] = 0x03007FD0;
    core->cpu.regs.usr.r13_usr = 0x03007F40;
    core->cpu.regs.usr.r14_usr = 0x08000050;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08009FFE, "PC 應回到 THUMB 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (CPU_STATE_SYSTEM | (1 << 5)), "CPSR 應恢復");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F40, "System SP 應恢復 User bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000050, "System LR 應恢復 User bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ABT_Return_WithFlags() {
    TEST_START("ABT 返回 ARM + CPSR 旗標");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_ABORT;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_ABORT | (1 << 7);
    core->cpu.regs.abt.spsr = (uint32_t)(CPU_STATE_SYSTEM | (1U << 31) | (1U << 30) | (1U << 29) | (1U << 28) | (1 << 7) | (1 << 6));
    core->cpu.regs.r[14] = 0x0800D004;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x0800D000, "PC 應回到 ARM 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (uint32_t)(CPU_STATE_SYSTEM | (1U << 31) | (1U << 30) | (1U << 29) | (1U << 28) | (1 << 7) | (1 << 6)), "CPSR 應恢復旗標與 I/F");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_UND_Return_SUBS_PC_LR() {
    TEST_START("UND 返回 SUBS PC, LR, #4");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_UNDEFINED;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_UNDEFINED | (1 << 7);
    core->cpu.regs.und.spsr = CPU_STATE_SUPERVISOR;
    core->cpu.regs.r[14] = 0x0800B004;
    core->cpu.regs.r[13] = 0x03007FB0;
    core->cpu.regs.svc.r13 = 0x03007FE0;
    core->cpu.regs.svc.r14 = 0x08000060;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x0800B000, "PC 應回到 ARM 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SUPERVISOR, "應回到 SVC 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, CPU_STATE_SUPERVISOR, "CPSR 應恢復");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FE0, "SVC SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000060, "SVC LR 應恢復 bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_UND_Return_To_THUMB_WithFlags() {
    TEST_START("UND 返回 THUMB + CPSR 旗標");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_UNDEFINED;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_UNDEFINED | (1 << 7);
    core->cpu.regs.und.spsr = (uint32_t)(CPU_STATE_SYSTEM | (1 << 5) | (1U << 31) | (1U << 28));
    core->cpu.regs.r[14] = 0x0800E003;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x0800DFFE, "PC 應回到 THUMB 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (uint32_t)(CPU_STATE_SYSTEM | (1 << 5) | (1U << 31) | (1U << 28)), "CPSR 應恢復旗標與 THUMB");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_Nested_IRQ_SVC_Returns() {
    TEST_START("中斷巢狀 IRQ <-> SVC 返回");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SYSTEM;
    core->cpu.regs.irq.r13 = 0x03007FA0;
    core->cpu.regs.irq.r14 = 0x08002004;
    core->cpu.regs.r[13] = 0x03007FA0;
    core->cpu.regs.r[14] = 0x08002004;
    core->cpu.regs.usr.r13_usr = 0x03007F00;
    core->cpu.regs.usr.r14_usr = 0x08000000;

    // 進入巢狀 SVC
    core->cpu.regs.svc.spsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.svc.r13 = 0x03007FE0;
    core->cpu.regs.svc.r14 = 0x08003004;
    GBA_CPU_SwitchMode(core, CPU_STATE_SUPERVISOR);
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR | (1 << 7);
    core->cpu.regs.r[14] = 0x08003004;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_IRQ, "應回到 IRQ 模式");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FA0, "IRQ SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08002004, "IRQ LR 應恢復 bank");

    // IRQ 返回到 System
    core->cpu.regs.irq.spsr = CPU_STATE_SYSTEM;
    core->cpu.regs.r[14] = 0x08002004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F00, "System SP 應恢復 User bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000000, "System LR 應恢復 User bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Return_To_System_THUMB_UserBank() {
    TEST_START("IRQ 返回 System+THUMB User bank 邊界");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_SYSTEM | (1 << 5) | (1 << 31) | (1 << 30) | (1 << 29) | (1 << 28);
    core->cpu.regs.r[14] = 0x08009004;

    core->cpu.regs.r[8] = 0xDEAD0008;
    core->cpu.regs.r[9] = 0xDEAD0009;
    core->cpu.regs.r[10] = 0xDEAD000A;
    core->cpu.regs.r[11] = 0xDEAD000B;
    core->cpu.regs.r[12] = 0xDEAD000C;

    core->cpu.regs.usr.r8_usr = 0xDEAD0008;
    core->cpu.regs.usr.r9_usr = 0xDEAD0009;
    core->cpu.regs.usr.r10_usr = 0xDEAD000A;
    core->cpu.regs.usr.r11_usr = 0xDEAD000B;
    core->cpu.regs.usr.r12_usr = 0xDEAD000C;
    core->cpu.regs.usr.r13_usr = 0x03007F20;
    core->cpu.regs.usr.r14_usr = 0x08000040;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08009000, "PC 應回到 THUMB 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_SYSTEM, "應回到 System 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (uint32_t)(CPU_STATE_SYSTEM | (1 << 5) | (1 << 31) | (1 << 30) | (1 << 29) | (1 << 28)), "CPSR 應恢復含旗標");
    ASSERT_EQ(core->cpu.regs.r[8], 0xDEAD0008, "System R8 應維持共享暫存器值");
    ASSERT_EQ(core->cpu.regs.r[9], 0xDEAD0009, "System R9 應維持共享暫存器值");
    ASSERT_EQ(core->cpu.regs.r[10], 0xDEAD000A, "System R10 應維持共享暫存器值");
    ASSERT_EQ(core->cpu.regs.r[11], 0xDEAD000B, "System R11 應維持共享暫存器值");
    ASSERT_EQ(core->cpu.regs.r[12], 0xDEAD000C, "System R12 應維持共享暫存器值");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007F20, "System SP 應恢復 User bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000040, "System LR 應恢復 User bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_SWI_Return_To_FIQ() {
    TEST_START("SWI 返回到 FIQ");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_SUPERVISOR | (1 << 7);
    core->cpu.regs.svc.spsr = CPU_STATE_FIQ | (1 << 5);  // 回到 FIQ + THUMB
    core->cpu.regs.r[14] = 0x08007003;
    core->cpu.regs.fiq.r13_fiq = 0x03007FF0;
    core->cpu.regs.fiq.r14_fiq = 0x08000030;
    core->cpu.regs.fiq.r8_fiq = 0xF8000008;
    core->cpu.regs.fiq.r9_fiq = 0xF9000009;
    core->cpu.regs.fiq.r10_fiq = 0xFA00000A;
    core->cpu.regs.fiq.r11_fiq = 0xFB00000B;
    core->cpu.regs.fiq.r12_fiq = 0xFC00000C;
    core->cpu.regs.usr.r8_usr = 0x08000008;
    core->cpu.regs.usr.r9_usr = 0x08000009;
    core->cpu.regs.usr.r10_usr = 0x0800000A;
    core->cpu.regs.usr.r11_usr = 0x0800000B;
    core->cpu.regs.usr.r12_usr = 0x0800000C;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.pc, 0x08006FFE, "PC 應回到 FIQ THUMB 位址");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切回 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_FIQ, "應回到 FIQ 模式");
    ASSERT_EQ(core->cpu.regs.cpsr, (CPU_STATE_FIQ | (1 << 5)), "CPSR 應恢復");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FF0, "FIQ SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000030, "FIQ LR 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[8], 0xF8000008, "FIQ R8 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[9], 0xF9000009, "FIQ R9 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[10], 0xFA00000A, "FIQ R10 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[11], 0xFB00000B, "FIQ R11 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[12], 0xFC00000C, "FIQ R12 應恢復 bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_IRQ_Return_To_FIQ_Banks() {
    TEST_START("IRQ 返回到 FIQ bank 一致性");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.priv_mode = CPU_STATE_IRQ;
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr = CPU_STATE_IRQ | (1 << 7);
    core->cpu.regs.irq.spsr = CPU_STATE_FIQ;  // 回到 FIQ
    core->cpu.regs.r[14] = 0x08008004;

    core->cpu.regs.fiq.r8_fiq = 0xF8000018;
    core->cpu.regs.fiq.r9_fiq = 0xF9000019;
    core->cpu.regs.fiq.r10_fiq = 0xFA00001A;
    core->cpu.regs.fiq.r11_fiq = 0xFB00001B;
    core->cpu.regs.fiq.r12_fiq = 0xFC00001C;
    core->cpu.regs.fiq.r13_fiq = 0x03007FF8;
    core->cpu.regs.fiq.r14_fiq = 0x08000038;
    core->cpu.regs.usr.r8_usr = 0x08000018;
    core->cpu.regs.usr.r9_usr = 0x08000019;
    core->cpu.regs.usr.r10_usr = 0x0800001A;
    core->cpu.regs.usr.r11_usr = 0x0800001B;
    core->cpu.regs.usr.r12_usr = 0x0800001C;

    uint32_t inst = 0xE25EF004;
    ARM_DataProcessing(core, inst);

    ASSERT_EQ(core->cpu.regs.priv_mode, CPU_STATE_FIQ, "應回到 FIQ 模式");
    ASSERT_EQ(core->cpu.regs.r[8], 0xF8000018, "FIQ R8 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[9], 0xF9000019, "FIQ R9 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[10], 0xFA00001A, "FIQ R10 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[11], 0xFB00001B, "FIQ R11 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[12], 0xFC00001C, "FIQ R12 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[13], 0x03007FF8, "FIQ SP 應恢復 bank");
    ASSERT_EQ(core->cpu.regs.r[14], 0x08000038, "FIQ LR 應恢復 bank");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

/* ============================================================================
 * ARM 分支指令測試
 * ============================================================================ */

bool Test_ARM_B() {
    TEST_START("ARM B (分支)");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.pc = 0x08000000;
    
    // B 向前跳 16 字節 (EA000002: offset = 2 words = 8 bytes)
    uint32_t inst = 0xEA000002;
    InstructionResult result = ARM_Branch(core, inst);
    
    ASSERT_TRUE(result.branch_taken, "應該發生分支");
    ASSERT_EQ(result.cycles, 3, "分支應消耗 3 週期");
    ASSERT_EQ(core->cpu.regs.pc & 0xFFFFFFFC, 0x08000008, "PC 應跳轉到正確位置");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_BL() {
    TEST_START("ARM BL (帶連結分支)");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.pc = 0x08000100;
    
    // BL 0x08000200 (EB000002)
    uint32_t inst = 0xEB000002;
    ARM_Branch(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[14], 0x080000FC, "LR 應保存返回地址");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_BX_THUMB_to_ARM_Align() {
    TEST_START("ARM BX (THUMB -> ARM 對齊)");
    GBA_Core *core = CreateTestCore();

    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr &= ~(1 << 5);
    core->cpu.regs.r[0] = 0x08004002;

    // BX R0 (E12FFF10)
    uint32_t inst = 0xE12FFF10;
    InstructionResult result = ARM_BranchExchange(core, inst);

    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "應保持 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x08004000, "PC 應對齊到 4 位元組");
    ASSERT_TRUE(result.branch_taken, "BX 應視為分支");
    ASSERT_TRUE(result.pipeline_flush, "BX 應刷新流水線");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_ARM_BX_ARM_to_THUMB() {
    TEST_START("ARM BX (ARM → THUMB)");
    GBA_Core *core = CreateTestCore();
    
    core->cpu.regs.r[0] = 0x08000101;  // 奇數地址 = THUMB
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    
    // BX R0 (E12FFF10)
    uint32_t inst = 0xE12FFF10;
    InstructionResult result = ARM_BranchExchange(core, inst);
    
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "應切換到 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x08000100, "PC 應對齊到偶數地址");
    ASSERT_TRUE(result.mode_changed, "應標記模式改變");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

/* ============================================================================
 * THUMB 指令測試
 * ============================================================================ */

bool Test_THUMB_MOV() {
    TEST_START("THUMB MOV 立即數");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    
    // MOVS R0, #42 (202A)
    uint16_t inst = 0x202A;
    THUMB_MoveCmpAddSubImm(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[0], 42, "R0 應等於 42");
    ASSERT_FALSE(GBA_CPU_GetFlag_N(&core->cpu), "N 標誌應清除");
    ASSERT_FALSE(GBA_CPU_GetFlag_Z(&core->cpu), "Z 標誌應清除");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_ADD() {
    TEST_START("THUMB ADD");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    
    core->cpu.regs.r[0] = 10;
    core->cpu.regs.r[1] = 20;
    
    // ADDS R2, R0, R1 (1842)
    uint16_t inst = 0x1842;
    THUMB_AddSubtract(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[2], 30, "10 + 20 = 30");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_SUB() {
    TEST_START("THUMB SUB");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    
    core->cpu.regs.r[0] = 50;
    core->cpu.regs.r[1] = 30;
    
    // SUBS R2, R0, R1 (1A42)
    uint16_t inst = 0x1A42;
    THUMB_AddSubtract(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[2], 20, "50 - 30 = 20");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_CMP() {
    TEST_START("THUMB CMP");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    
    core->cpu.regs.r[0] = 42;
    
    // CMP R0, #42 (282A)
    uint16_t inst = 0x282A;
    THUMB_MoveCmpAddSubImm(core, inst);
    
    ASSERT_TRUE(GBA_CPU_GetFlag_Z(&core->cpu), "Z 標誌應設置（相等）");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_PCRelativeLoad_Alignment() {
    TEST_START("THUMB PC 相對載入對齊");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.pc = 0x03000002;
    GBA_MemoryWrite32(core, 0x03000008, 0xAABBCCDD);

    // LDR R0, [PC, #4] (4801)
    uint16_t inst = 0x4801;
    THUMB_PCRelativeLoad(core, inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0xAABBCCDD, "PC 相對載入應對齊到 4 位址");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_HiRegisterOps_FlagsUnchanged() {
    TEST_START("THUMB 高位暫存器 flags 不變");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    SetFlags(core, true, false, true, false);
    core->cpu.regs.r[0] = 0x00000001;
    core->cpu.regs.r[8] = 0x00000002;
    core->cpu.regs.r[9] = 0x00000003;
    core->cpu.regs.r[10] = 0x00000004;

    uint16_t mov_r8_r0 = (uint16_t)(0x4400 | (2 << 8) | (1 << 7) | (0 << 6) | (0 << 3) | 0);
    THUMB_HiRegisterOps(core, mov_r8_r0);
    ASSERT_TRUE(GBA_CPU_GetFlag_N(&core->cpu), "MOV 不應改變 N");
    ASSERT_FALSE(GBA_CPU_GetFlag_Z(&core->cpu), "MOV 不應改變 Z");
    ASSERT_TRUE(GBA_CPU_GetFlag_C(&core->cpu), "MOV 不應改變 C");
    ASSERT_FALSE(GBA_CPU_GetFlag_V(&core->cpu), "MOV 不應改變 V");

    uint16_t add_r9_r10 = (uint16_t)(0x4400 | (0 << 8) | (1 << 7) | (1 << 6) | (2 << 3) | 1);
    THUMB_HiRegisterOps(core, add_r9_r10);
    ASSERT_TRUE(GBA_CPU_GetFlag_N(&core->cpu), "ADD 不應改變 N");
    ASSERT_FALSE(GBA_CPU_GetFlag_Z(&core->cpu), "ADD 不應改變 Z");
    ASSERT_TRUE(GBA_CPU_GetFlag_C(&core->cpu), "ADD 不應改變 C");
    ASSERT_FALSE(GBA_CPU_GetFlag_V(&core->cpu), "ADD 不應改變 V");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_HiRegisterOps() {
    TEST_START("THUMB 高位暫存器操作");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[0] = 0x11111111;
    core->cpu.regs.r[8] = 0x22222222;
    core->cpu.regs.r[9] = 0x00000007;
    core->cpu.regs.r[10] = 0x00000005;

    // MOV R8, R0 (010001 10 1 0 000 000)
    uint16_t mov_r8_r0 = (uint16_t)(0x4400 | (2 << 8) | (1 << 7) | (0 << 6) | (0 << 3) | 0);
    THUMB_HiRegisterOps(core, mov_r8_r0);
    ASSERT_EQ(core->cpu.regs.r[8], 0x11111111, "MOV 應寫入高位暫存器");

    // MOV R0, R8 (010001 10 0 1 000 000)
    uint16_t mov_r0_r8 = (uint16_t)(0x4400 | (2 << 8) | (0 << 7) | (1 << 6) | (0 << 3) | 0);
    THUMB_HiRegisterOps(core, mov_r0_r8);
    ASSERT_EQ(core->cpu.regs.r[0], 0x11111111, "MOV 應從高位暫存器讀回");

    // ADD R9, R10 (010001 00 1 1 010 001)
    uint16_t add_r9_r10 = (uint16_t)(0x4400 | (0 << 8) | (1 << 7) | (1 << 6) | (2 << 3) | 1);
    THUMB_HiRegisterOps(core, add_r9_r10);
    ASSERT_EQ(core->cpu.regs.r[9], 0x0000000C, "ADD 高位暫存器應相加");

    // CMP R9, R10 (010001 01 1 1 010 001)
    uint16_t cmp_r9_r10 = (uint16_t)(0x4400 | (1 << 8) | (1 << 7) | (1 << 6) | (2 << 3) | 1);
    THUMB_HiRegisterOps(core, cmp_r9_r10);
    ASSERT_FALSE(GBA_CPU_GetFlag_Z(&core->cpu), "CMP 應清除 Z");
    ASSERT_FALSE(GBA_CPU_GetFlag_N(&core->cpu), "CMP 結果為正，N 應清除");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_BX_HighReg_To_ARM() {
    TEST_START("THUMB BX 高位暫存器 -> ARM");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 5);

    core->cpu.regs.r[8] = 0x08001234;

    // BX R8 (010001 11 0 1 000 000)
    uint16_t bx_r8 = (uint16_t)(0x4400 | (3 << 8) | (0 << 7) | (1 << 6) | (0 << 3) | 0);
    InstructionResult result = THUMB_HiRegisterOps(core, bx_r8);

    ASSERT_TRUE(result.branch_taken, "BX 應視為分支");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_ARM, "BX 應切換到 ARM 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x08001234, "PC 應對齊到 4 位址");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_BX_LowReg_To_THUMB() {
    TEST_START("THUMB BX 低位暫存器 -> THUMB");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 5);

    core->cpu.regs.r[0] = 0x08001235;

    // BX R0 (010001 11 0 0 000 000)
    uint16_t bx_r0 = (uint16_t)(0x4400 | (3 << 8) | (0 << 7) | (0 << 6) | (0 << 3) | 0);
    InstructionResult result = THUMB_HiRegisterOps(core, bx_r0);

    ASSERT_TRUE(result.branch_taken, "BX 應視為分支");
    ASSERT_EQ(core->cpu.regs.exec_mode, CPU_MODE_THUMB, "BX 應維持 THUMB 模式");
    ASSERT_EQ(core->cpu.regs.pc, 0x08001234, "PC 應對齊到 2 位址");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_LDR_STR_Imm() {
    TEST_START("THUMB LDR/STR 立即數");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[0] = 0xDEADBEEF;

    // STR R0, [R1, #4] (6008 with imm5=1)
    uint16_t str_inst = 0x6008;
    THUMB_LoadStoreImmOffset(core, str_inst);

    // LDR R2, [R1, #4] (680A with imm5=1)
    uint16_t ldr_inst = 0x680A;
    THUMB_LoadStoreImmOffset(core, ldr_inst);

    ASSERT_EQ(core->cpu.regs.r[2], 0xDEADBEEF, "THUMB LDR 應讀回寫入的值");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_LDRH_STRH() {
    TEST_START("THUMB LDRH/STRH");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[0] = 0x0000ABCD;

    // STRH R0, [R1, #2] (8048)
    uint16_t strh_inst = 0x8048;
    THUMB_LoadStoreHalfword(core, strh_inst);

    // LDRH R2, [R1, #2] (884A)
    uint16_t ldrh_inst = 0x884A;
    THUMB_LoadStoreHalfword(core, ldrh_inst);

    ASSERT_EQ(core->cpu.regs.r[2], 0x0000ABCD, "LDRH 應讀回 16 位數據");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_LDRH_Unaligned() {
    TEST_START("THUMB LDRH 非對齊");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000001;  // 故意設為非對齊
    GBA_MemoryWrite16(core, 0x03000000, 0x1234);

    // LDRH R0, [R1, #0] (8808)
    uint16_t ldrh_inst = 0x8808;
    THUMB_LoadStoreHalfword(core, ldrh_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0x00001234, "LDRH 應對齊到偶數位址讀取");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_STRH_Unaligned() {
    TEST_START("THUMB STRH 非對齊");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000001;  // 故意設為非對齊
    core->cpu.regs.r[0] = 0x0000BEEF;
    GBA_MemoryWrite16(core, 0x03000000, 0x0000);

    // STRH R0, [R1, #0] (8008)
    uint16_t strh_inst = 0x8008;
    THUMB_LoadStoreHalfword(core, strh_inst);

    ASSERT_EQ(GBA_MemoryRead16(core, 0x03000000), 0xBEEF, "STRH 應對齊到偶數位址寫入");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_STRB_LDRB_Unaligned_Boundary() {
    TEST_START("THUMB STRB/LDRB 非對齊與邊界");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    // 非對齊存取 (對 byte 無影響)
    core->cpu.regs.r[1] = 0x03000003;
    core->cpu.regs.r[0] = 0x0000005A;
    GBA_MemoryWrite8(core, 0x03000003, 0x00);

    // STRB R0, [R1, #0] (7008)
    uint16_t strb_inst = 0x7008;
    THUMB_LoadStoreImmOffset(core, strb_inst);
    ASSERT_EQ(GBA_MemoryRead8(core, 0x03000003), 0x5A, "STRB 應寫入 byte");

    // LDRB R2, [R1, #0] (780A)
    uint16_t ldrb_inst = 0x780A;
    THUMB_LoadStoreImmOffset(core, ldrb_inst);
    ASSERT_EQ(core->cpu.regs.r[2], 0x0000005A, "LDRB 應讀回 byte");

    // 邊界：IWRAM 最後一個位址
    core->cpu.regs.r[1] = 0x03007FFF;
    core->cpu.regs.r[0] = 0x000000A5;

    THUMB_LoadStoreImmOffset(core, strb_inst);
    THUMB_LoadStoreImmOffset(core, ldrb_inst);
    ASSERT_EQ(core->cpu.regs.r[2], 0x000000A5, "邊界位址讀寫應正確");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_LDR_STR_ROM_VRAM() {
    TEST_START("THUMB LDR/STR ROM/VRAM 行為");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    // ROM 讀取與寫入忽略
    core->memory.rom_size = 16;
    core->memory.rom = (uint8_t *)malloc(core->memory.rom_size);
    memset(core->memory.rom, 0, core->memory.rom_size);
    core->memory.rom[0] = 0x78;
    core->memory.rom[1] = 0x56;
    core->memory.rom[2] = 0x34;
    core->memory.rom[3] = 0x12;

    core->cpu.regs.r[1] = 0x08000000;
    core->cpu.regs.r[0] = 0xAABBCCDD;

    // STR R0, [R1, #0] (6008) - ROM 應忽略
    uint16_t str_inst = 0x6008;
    THUMB_LoadStoreImmOffset(core, str_inst);

    // LDR R2, [R1, #0] (680A)
    uint16_t ldr_inst = 0x680A;
    THUMB_LoadStoreImmOffset(core, ldr_inst);
    ASSERT_EQ(core->cpu.regs.r[2], 0x12345678, "ROM 應保持原值");

    // VRAM 讀寫
    core->cpu.regs.r[1] = 0x06000000;
    core->cpu.regs.r[0] = 0x0BADF00D;
    THUMB_LoadStoreImmOffset(core, str_inst);
    THUMB_LoadStoreImmOffset(core, ldr_inst);
    ASSERT_EQ(core->cpu.regs.r[2], 0x0BADF00D, "VRAM 應可寫入並讀回");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_LDR_UnalignedRotate() {
    TEST_START("THUMB LDR 非對齊旋轉");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[2] = 1;
    GBA_MemoryWrite32(core, 0x03000000, 0x11223344);

    // LDR R0, [R1, R2] (5A88)
    uint16_t ldr_inst = 0x5A88;
    THUMB_LoadStoreRegOffset(core, ldr_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0x44112233, "非對齊 LDR 應旋轉 8 位");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_LDR_RegOffset_UnalignedRotate_Multi() {
    TEST_START("THUMB LDR RegOffset 未對齊旋轉 (多偏移)");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000000;
    GBA_MemoryWrite32(core, 0x03000000, 0xA1B2C3D4);

    // LDR R0, [R1, R2] (5A88)
    uint16_t ldr_inst = 0x5A88;

    core->cpu.regs.r[2] = 1;
    THUMB_LoadStoreRegOffset(core, ldr_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0xD4A1B2C3, "位址+1 應旋轉 8 位");

    core->cpu.regs.r[2] = 2;
    THUMB_LoadStoreRegOffset(core, ldr_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0xC3D4A1B2, "位址+2 應旋轉 16 位");

    core->cpu.regs.r[2] = 3;
    THUMB_LoadStoreRegOffset(core, ldr_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0xB2C3D4A1, "位址+3 應旋轉 24 位");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_STR_RegOffset_UnalignedAlign_Word() {
    TEST_START("THUMB STR RegOffset 未對齊對齊回寫 (Word)");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000000;
    core->cpu.regs.r[2] = 1;
    core->cpu.regs.r[0] = 0xCAFEBABE;

    // STR R0, [R1, R2] (5800 + Rm/Rn/Rd)
    uint16_t str_inst = (uint16_t)(0x5800 | (2 << 6) | (1 << 3) | 0);
    THUMB_LoadStoreRegOffset(core, str_inst);

    ASSERT_EQ(GBA_MemoryRead32(core, 0x03000000), 0xCAFEBABE, "STR 應對齊到 4 位址寫入");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_SignedLoad() {
    TEST_START("THUMB LDRSB/LDRSH");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[1] = 0x03000010;
    core->cpu.regs.r[2] = 4;

    // 寫入測試資料
    GBA_MemoryWrite8(core, 0x03000014, 0x80);
    GBA_MemoryWrite16(core, 0x03000014, 0xFF80);

    // LDRSB R0, [R1, R2] (5C88)
    uint16_t ldrsb_inst = 0x5C88;
    THUMB_LoadStoreSigned(core, ldrsb_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0xFFFFFF80, "LDRSB 應符號擴展");

    // LDRSH R0, [R1, R2] (5E88)
    uint16_t ldrsh_inst = 0x5E88;
    THUMB_LoadStoreSigned(core, ldrsh_inst);
    ASSERT_EQ(core->cpu.regs.r[0], 0xFFFFFF80, "LDRSH 應符號擴展");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

bool Test_THUMB_PUSH_POP() {
    TEST_START("THUMB PUSH/POP");
    GBA_Core *core = CreateTestCore();
    core->cpu.regs.exec_mode = CPU_MODE_THUMB;

    core->cpu.regs.r[0] = 0x11111111;
    core->cpu.regs.r[1] = 0x22222222;
    core->cpu.regs.r[13] = 0x03000020;

    // PUSH {R0,R1} (B403)
    uint16_t push_inst = 0xB403;
    THUMB_PushPop(core, push_inst);

    core->cpu.regs.r[0] = 0;
    core->cpu.regs.r[1] = 0;

    // POP {R0,R1} (BC03)
    uint16_t pop_inst = 0xBC03;
    THUMB_PushPop(core, pop_inst);

    ASSERT_EQ(core->cpu.regs.r[0], 0x11111111, "POP 應載入 R0");
    ASSERT_EQ(core->cpu.regs.r[1], 0x22222222, "POP 應載入 R1");

    GBA_CoreDestroy(core);
    TEST_PASS();
}

/* ============================================================================
 * 條件執行測試
 * ============================================================================ */

bool Test_ARM_Conditional_EQ() {
    TEST_START("ARM 條件執行 (EQ)");
    GBA_Core *core = CreateTestCore();
    
    // 設置 Z 標誌
    SetFlags(core, false, true, false, false);
    
    core->cpu.regs.r[0] = 0;
    
    // MOVEQ R0, #1 (03A00001) - 應執行
    uint32_t inst = 0x03A00001;
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[0], 1, "EQ 條件滿足時應執行");
    
    // 清除 Z 標誌
    SetFlags(core, false, false, false, false);
    
    core->cpu.regs.r[0] = 0;
    
    // MOVEQ R0, #1 - 不應執行
    ARM_DataProcessing(core, inst);
    
    ASSERT_EQ(core->cpu.regs.r[0], 0, "EQ 條件不滿足時不應執行");
    
    GBA_CoreDestroy(core);
    TEST_PASS();
}

/* ============================================================================
 * 主測試運行器
 * ============================================================================ */

void RunAllTests() {
    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   GBA CPU 指令測試套件                      ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    
    // ARM 數據處理指令
    printf("\n【ARM 數據處理指令】\n");
    Test_ARM_ADD();
    Test_ARM_SUB();
    Test_ARM_MOV();
    Test_ARM_MOV_Immediate();
    Test_ARM_CMP();
    Test_ARM_AND();
    Test_ARM_ORR();
    Test_ARM_STR_LDR_Immediate();
    Test_ARM_LDR_STR_MemoryBoundaries();
    Test_ARM_IORegister_SideEffects();
    Test_VRAM_OAM_Mirror_Align();
    Test_ARM_LDR_UnalignedRotate_Word();
    Test_ARM_STR_UnalignedAlign_Word();
    Test_ARM_STRB_LDRB();
    Test_ARM_STR_PostIndex_Writeback();
    Test_ARM_MUL_MLA();
    Test_ARM_MULLong();
    Test_ARM_LDM_STM();
    Test_ARM_LDM_STM_Decrement();
    Test_ARM_LDM_PC_Writeback();
    Test_ARM_LDM_RnInList_NoWriteback();
    Test_ARM_STM_PC_Writeback();
    Test_ARM_LDM_S_RestoreCPSR();
    Test_ARM_STM_S_UserBank();
    Test_ARM_LDM_S_UserBank();
    Test_ARM_STM_S_UserBank_FIQ_R8_R12();
    Test_ARM_LDM_S_UserBank_FIQ_R8_R12();
    Test_ARM_STM_S_UserBank_IRQ_SVC();
    Test_ARM_LDM_S_UserBank_IRQ_SVC();
    Test_ARM_STM_S_Writeback_RnInList();
    Test_ARM_LDM_S_RnInList_NoWriteback();
    Test_ARM_LDM_S_PC_RnInList_NoWriteback();
    Test_ARM_LDM_S_PC_NoUserBankLoad();
    Test_ARM_LDM_PC_PipelineFlush();
    Test_ARM_LDM_S_PC_SwitchBank_SVC();
    Test_ARM_LDM_S_PC_SwitchBank_FIQ();
    Test_CPU_ModeSwitch_IRQ_SVC_FIQ();
    Test_CPU_ModeSwitch_FIQ_System_RoundTrip();
    Test_FIQ_From_THUMB();
    Test_FIQ_From_ARM();
    Test_IRQ_Masked_By_I();
    Test_FIQ_Masked_By_F();
    Test_IRQ_From_ARM();
    Test_IRQ_From_THUMB();
    Test_IRQ_Vector_PipelineFlush_ARM_Boundary();
    Test_IRQ_Vector_PipelineFlush_THUMB_Boundary();
    Test_FIQ_Priority_Over_IRQ_Vector();
    Test_SWI_From_ARM();
    Test_SWI_From_THUMB();
    Test_IRQ_Return_SUBS_PC_LR();
    Test_SWI_Return_SUBS_PC_LR();
    Test_SWI_Return_To_THUMB();
    Test_IRQ_Return_To_ARM();
    Test_SWI_Return_To_ARM_WithFlags();
    Test_IRQ_Return_To_SVC();
    Test_IRQ_Return_To_System_THUMB_UserBank();
    Test_IRQ_Return_CPSR_Flags_IF();
    Test_ABT_Return_SUBS_PC_LR();
    Test_ABT_Return_WithFlags();
    Test_UND_Return_SUBS_PC_LR();
    Test_UND_Return_To_THUMB_WithFlags();
    Test_Nested_IRQ_SVC_Returns();
    Test_SWI_Return_To_FIQ();
    Test_IRQ_Return_To_FIQ_Banks();
    
    // ARM 分支指令
    printf("\n【ARM 分支指令】\n");
    Test_ARM_B();
    Test_ARM_BL();
    Test_ARM_BX_ARM_to_THUMB();
    Test_ARM_BX_THUMB_to_ARM_Align();
    
    // THUMB 指令
    printf("\n【THUMB 指令】\n");
    Test_THUMB_MOV();
    Test_THUMB_ADD();
    Test_THUMB_SUB();
    Test_THUMB_CMP();
    Test_THUMB_PCRelativeLoad_Alignment();
    Test_THUMB_HiRegisterOps();
    Test_THUMB_HiRegisterOps_FlagsUnchanged();
    Test_THUMB_BX_HighReg_To_ARM();
    Test_THUMB_BX_LowReg_To_THUMB();
    Test_THUMB_LDR_STR_Imm();
    Test_THUMB_LDRH_STRH();
    Test_THUMB_LDRH_Unaligned();
    Test_THUMB_STRH_Unaligned();
    Test_THUMB_STRB_LDRB_Unaligned_Boundary();
    Test_THUMB_LDR_STR_ROM_VRAM();
    Test_THUMB_LDR_UnalignedRotate();
    Test_THUMB_LDR_RegOffset_UnalignedRotate_Multi();
    Test_THUMB_STR_RegOffset_UnalignedAlign_Word();
    Test_THUMB_SignedLoad();
    Test_THUMB_PUSH_POP();
    
    // 條件執行
    printf("\n【條件執行】\n");
    Test_ARM_Conditional_EQ();
    
    // 總結
    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   測試結果                                   ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║   總測試數: %-4d                            ║\n", tests_run);
    printf("║   通過:     %-4d                            ║\n", tests_passed);
    printf("║   失敗:     %-4d                            ║\n", tests_failed);
    printf("║   通過率:   %.1f%%                          ║\n", 
           tests_run > 0 ? (tests_passed * 100.0 / tests_run) : 0.0);
    printf("╚══════════════════════════════════════════════╝\n");
    
    if (tests_failed == 0) {
        printf("\n🎉 所有測試通過！\n\n");
    } else {
        printf("\n⚠️  有 %d 個測試失敗，請檢查\n\n", tests_failed);
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    RunAllTests();
    return tests_failed > 0 ? 1 : 0;
}
