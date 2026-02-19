/**
 * GBA CPU 指令處理 - 現代化設計
 * 
 * 設計特點：
 * 1. 查找表驅動 - 快速指令解碼
 * 2. 統一接口 - 所有指令函數返回 InstructionResult
 * 3. 上下文傳遞 - 無全局變數依賴
 * 4. 高性能 - 內聯關鍵路徑
 */

#ifndef GBA_CPU_INSTRUCTIONS_H
#define GBA_CPU_INSTRUCTIONS_H

#include "gba_core.h"

/* ============================================================================
 * 指令類型定義
 * ============================================================================ */

// ARM 指令格式
typedef enum {
    ARM_DATA_PROC,         // 數據處理
    ARM_PSR_TRANSFER,      // PSR 傳送
    ARM_MULTIPLY,          // 乘法
    ARM_MULTIPLY_LONG,     // 長乘法
    ARM_SINGLE_DATA_SWAP,  // 單數據交換
    ARM_BRANCH_EXCHANGE,   // 分支交換
    ARM_HALFWORD_DT,       // 半字數據傳送
    ARM_SINGLE_DATA_TRANSFER,  // 單數據傳送
    ARM_UNDEFINED,         // 未定義
    ARM_BLOCK_DATA_TRANSFER,   // 批量數據傳送
    ARM_BRANCH,            // 分支
    ARM_COPROCESSOR_DT,    // 協處理器數據傳送 (GBA 不支持)
    ARM_COPROCESSOR_DO,    // 協處理器操作 (GBA 不支持)
    ARM_COPROCESSOR_RT,    // 協處理器寄存器傳送 (GBA 不支持)
    ARM_SOFTWARE_INTERRUPT // 軟件中斷
} ARM_InstructionType;

// THUMB 指令格式
typedef enum {
    THUMB_MOVE_SHIFTED_REG,
    THUMB_ADD_SUB,
    THUMB_MOV_CMP_ADD_SUB_IMM,
    THUMB_ALU_OP,
    THUMB_HI_REG_OP,
    THUMB_PC_REL_LOAD,
    THUMB_LOAD_STORE_REG_OFFSET,
    THUMB_LOAD_STORE_SIGNED,
    THUMB_LOAD_STORE_IMM_OFFSET,
    THUMB_LOAD_STORE_HALFWORD,
    THUMB_SP_REL_LOAD_STORE,
    THUMB_LOAD_ADDRESS,
    THUMB_ADD_OFFSET_TO_SP,
    THUMB_PUSH_POP,
    THUMB_MULTIPLE_LOAD_STORE,
    THUMB_CONDITIONAL_BRANCH,
    THUMB_SOFTWARE_INTERRUPT,
    THUMB_UNCONDITIONAL_BRANCH,
    THUMB_LONG_BRANCH_LINK
} THUMB_InstructionType;

/* ============================================================================
 * 指令元數據 (用於調試和優化)
 * ============================================================================ */
typedef struct {
    const char *mnemonic;      // 助記符 (如 "ADD")
    uint8_t     min_cycles;    // 最小周期
    uint8_t     max_cycles;    // 最大周期
    bool        can_branch;    // 是否可能分支
    bool        can_load;      // 是否載入記憶體
    bool        can_store;     // 是否存儲記憶體
} InstructionInfo;

/* ============================================================================
 * 條件碼
 * ============================================================================ */
typedef enum {
    COND_EQ = 0b0000,  // Equal (Z set)
    COND_NE = 0b0001,  // Not equal (Z clear)
    COND_CS = 0b0010,  // Carry set
    COND_CC = 0b0011,  // Carry clear
    COND_MI = 0b0100,  // Minus/negative (N set)
    COND_PL = 0b0101,  // Plus/positive (N clear)
    COND_VS = 0b0110,  // Overflow set (V set)
    COND_VC = 0b0111,  // Overflow clear (V clear)
    COND_HI = 0b1000,  // Unsigned higher (C set and Z clear)
    COND_LS = 0b1001,  // Unsigned lower or same (C clear or Z set)
    COND_GE = 0b1010,  // Signed greater than or equal (N == V)
    COND_LT = 0b1011,  // Signed less than (N != V)
    COND_GT = 0b1100,  // Signed greater than (Z clear and N == V)
    COND_LE = 0b1101,  // Signed less than or equal (Z set or N != V)
    COND_AL = 0b1110,  // Always
    COND_NV = 0b1111   // Never (ARMv1-3) / Extended (ARMv4+)
} ARM_ConditionCode;

/* ============================================================================
 * 數據處理操作碼
 * ============================================================================ */
typedef enum {
    DP_AND = 0x0,  // Logical AND
    DP_EOR = 0x1,  // Logical XOR
    DP_SUB = 0x2,  // Subtract
    DP_RSB = 0x3,  // Reverse subtract
    DP_ADD = 0x4,  // Add
    DP_ADC = 0x5,  // Add with carry
    DP_SBC = 0x6,  // Subtract with carry
    DP_RSC = 0x7,  // Reverse subtract with carry
    DP_TST = 0x8,  // Test (AND, discard result)
    DP_TEQ = 0x9,  // Test equivalence (XOR, discard result)
    DP_CMP = 0xA,  // Compare (SUB, discard result)
    DP_CMN = 0xB,  // Compare negated (ADD, discard result)
    DP_ORR = 0xC,  // Logical OR
    DP_MOV = 0xD,  // Move
    DP_BIC = 0xE,  // Bit clear
    DP_MVN = 0xF   // Move NOT
} DataProcessingOpcode;

/* ============================================================================
 * 指令函數類型定義
 * ============================================================================ */
typedef InstructionResult (*ARM_InstructionHandler)(GBA_Core *core, uint32_t instruction);
typedef InstructionResult (*THUMB_InstructionHandler)(GBA_Core *core, uint16_t instruction);

/* ============================================================================
 * 主執行函數
 * ============================================================================ */

/**
 * 執行單條 ARM 指令
 */
InstructionResult GBA_CPU_ExecuteARM(GBA_Core *core, uint32_t instruction);

/**
 * 執行單條 THUMB 指令
 */
InstructionResult GBA_CPU_ExecuteTHUMB(GBA_Core *core, uint16_t instruction);

/* ============================================================================
 * ARM 指令處理函數
 * ============================================================================ */

InstructionResult ARM_DataProcessing(GBA_Core *core, uint32_t inst);
InstructionResult ARM_PSRTransfer(GBA_Core *core, uint32_t inst);
InstructionResult ARM_Multiply(GBA_Core *core, uint32_t inst);
InstructionResult ARM_MultiplyLong(GBA_Core *core, uint32_t inst);
InstructionResult ARM_SingleDataSwap(GBA_Core *core, uint32_t inst);
InstructionResult ARM_BranchExchange(GBA_Core *core, uint32_t inst);
InstructionResult ARM_HalfwordDataTransfer(GBA_Core *core, uint32_t inst);
InstructionResult ARM_SingleDataTransfer(GBA_Core *core, uint32_t inst);
InstructionResult ARM_Undefined(GBA_Core *core, uint32_t inst);
InstructionResult ARM_BlockDataTransfer(GBA_Core *core, uint32_t inst);
InstructionResult ARM_Branch(GBA_Core *core, uint32_t inst);
InstructionResult ARM_SoftwareInterrupt(GBA_Core *core, uint32_t inst);

/* ============================================================================
 * THUMB 指令處理函數
 * ============================================================================ */

InstructionResult THUMB_MoveShiftedRegister(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_AddSubtract(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_MoveCmpAddSubImm(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_ALUOperation(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_HiRegisterOps(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_PCRelativeLoad(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_LoadStoreRegOffset(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_LoadStoreSigned(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_LoadStoreImmOffset(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_LoadStoreHalfword(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_SPRelativeLoadStore(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_LoadAddress(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_AddOffsetToSP(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_PushPop(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_MultipleLoadStore(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_ConditionalBranch(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_SoftwareInterrupt(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_UnconditionalBranch(GBA_Core *core, uint16_t inst);
InstructionResult THUMB_LongBranchLink(GBA_Core *core, uint16_t inst);

/* ============================================================================
 * 輔助函數
 * ============================================================================ */

/**
 * 檢查 ARM 指令條件碼
 * @return true 表示條件滿足，應執行指令
 */
bool GBA_CPU_CheckCondition(const GBA_Core *core, ARM_ConditionCode cond);

/**
 * 解碼 ARM 指令類型
 */
ARM_InstructionType GBA_CPU_DecodeARM(uint32_t instruction);

/**
 * 解碼 THUMB 指令類型
 */
THUMB_InstructionType GBA_CPU_DecodeTHUMB(uint16_t instruction);

/**
 * 獲取指令元數據 (調試用)
 */
const InstructionInfo* GBA_CPU_GetInstructionInfo(uint32_t instruction, bool is_thumb);

/* ============================================================================
 * 內聯性能關鍵函數
 * ============================================================================ */

// 條件檢查 (高度優化版本)
static inline bool GBA_CPU_CheckConditionFast(uint32_t cpsr, uint8_t cond) {
    // 使用查找表加速條件檢查
    static const uint16_t condition_lut[16] = {
        0xF0F0,  // EQ
        0x0F0F,  // NE
        0xCCCC,  // CS/HS
        0x3333,  // CC/LO
        0xFF00,  // MI
        0x00FF,  // PL
        0xAAAA,  // VS
        0x5555,  // VC
        0x0C0C,  // HI
        0xF3F3,  // LS
        0xAA55,  // GE
        0x55AA,  // LT
        0x0A05,  // GT
        0xF5FA,  // LE
        0xFFFF,  // AL
        0x0000   // NV
    };
    
    uint8_t flags = (cpsr >> 28) & 0xF;
    return (condition_lut[cond] >> flags) & 1;
}

// 位域提取 (編譯器會優化為位操作)
static inline uint32_t BITS(uint32_t value, uint8_t high, uint8_t low) {
    return (value >> low) & ((1U << (high - low + 1)) - 1);
}

// 符號擴展
static inline int32_t SIGN_EXTEND(uint32_t value, uint8_t bits) {
    uint32_t sign_bit = 1U << (bits - 1);
    if (value & sign_bit) {
        return value | ~((1U << bits) - 1);
    }
    return value;
}

// 循環右移
static inline uint32_t ROR(uint32_t value, uint8_t amount) {
    amount &= 31;
    return (value >> amount) | (value << (32 - amount));
}

// 算術右移
static inline int32_t ASR(int32_t value, uint8_t amount) {
    if (amount >= 32) {
        return value < 0 ? -1 : 0;
    }
    return value >> amount;
}

// 快速寄存器訪問
static inline uint32_t GBA_CPU_GetReg(const GBA_Core *core, uint8_t reg) {
    if (reg < 15) {
        return core->cpu.regs.r[reg];
    }
    // PC 已經包含 pipeline offset，直接返回
    return core->cpu.regs.pc;
}

static inline void GBA_CPU_SetReg(GBA_Core *core, uint8_t reg, uint32_t value) {
    if (reg < 15) {
        core->cpu.regs.r[reg] = value;
    } else {
        // 設置執行地址，並同步更新 PC
        if (core->cpu.regs.exec_mode == CPU_MODE_THUMB) {
            core->cpu.regs.exec_addr = value & ~1;  // THUMB 對齊到 2
            core->cpu.regs.pc = core->cpu.regs.exec_addr + 4;
        } else {
            core->cpu.regs.exec_addr = value & ~3;  // ARM 對齊到 4
            core->cpu.regs.pc = core->cpu.regs.exec_addr + 8;
        }
    }
}

/* ============================================================================
 * 查找表 (在 .c 文件中定義)
 * ============================================================================ */

// ARM 指令解碼查找表 (4096 項，覆蓋所有可能的指令模式)
extern const ARM_InstructionHandler arm_instruction_lut[4096];

// THUMB 指令解碼查找表 (256 項)
extern const THUMB_InstructionHandler thumb_instruction_lut[256];

// 指令元數據表
extern const InstructionInfo arm_instruction_info[4096];
extern const InstructionInfo thumb_instruction_info[256];

#endif // GBA_CPU_INSTRUCTIONS_H
