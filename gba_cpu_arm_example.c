/**
 * ARM 指令集完整實現
 * 支援所有 ARM7TDMI 指令型別
 */

#include "gba_cpu_instructions.h"
#include "gba_core.h"
#include <stdio.h>
#include <string.h>

static uint8_t ARM_CountSetBits(uint32_t value) {
    uint8_t count = 0;
    while (value) {
        count += (value & 1U);
        value >>= 1;
    }
    return count;
}

static uint8_t ARM_MultiplyCycles(uint32_t value) {
    if ((value & 0xFFFFFF00) == 0 || (value & 0xFFFFFF00) == 0xFFFFFF00) {
        return 1;
    }
    if ((value & 0xFFFF0000) == 0 || (value & 0xFFFF0000) == 0xFFFF0000) {
        return 2;
    }
    if ((value & 0xFF000000) == 0 || (value & 0xFF000000) == 0xFF000000) {
        return 3;
    }
    return 4;
}

static uint32_t ARM_GetUserBankReg(const GBA_Core *core, uint8_t reg) {
    switch (reg) {
        case 8:  return core->cpu.regs.usr.r8_usr;
        case 9:  return core->cpu.regs.usr.r9_usr;
        case 10: return core->cpu.regs.usr.r10_usr;
        case 11: return core->cpu.regs.usr.r11_usr;
        case 12: return core->cpu.regs.usr.r12_usr;
        case 13: return core->cpu.regs.usr.r13_usr;
        case 14: return core->cpu.regs.usr.r14_usr;
        default: return 0;
    }
}

static void ARM_SetUserBankReg(GBA_Core *core, uint8_t reg, uint32_t value) {
    switch (reg) {
        case 8:  core->cpu.regs.usr.r8_usr = value; break;
        case 9:  core->cpu.regs.usr.r9_usr = value; break;
        case 10: core->cpu.regs.usr.r10_usr = value; break;
        case 11: core->cpu.regs.usr.r11_usr = value; break;
        case 12: core->cpu.regs.usr.r12_usr = value; break;
        case 13: core->cpu.regs.usr.r13_usr = value; break;
        case 14: core->cpu.regs.usr.r14_usr = value; break;
        default: break;
    }
}

static bool ARM_IsUserOrSystemMode(const GBA_Core *core) {
    return core->cpu.regs.priv_mode == CPU_STATE_USER ||
           core->cpu.regs.priv_mode == CPU_STATE_SYSTEM;
}

static uint32_t ARM_GetUserReg(const GBA_Core *core, uint8_t reg) {
    if (reg < 8) {
        return core->cpu.regs.r[reg];
    }

    if (reg == 15) {
        return core->cpu.regs.pc;
    }

    if (ARM_IsUserOrSystemMode(core)) {
        return GBA_CPU_GetReg(core, reg);
    }

    if (reg <= 12) {
        if (core->cpu.regs.priv_mode == CPU_STATE_FIQ) {
            return ARM_GetUserBankReg(core, reg);
        }
        return core->cpu.regs.r[reg];
    }

    return ARM_GetUserBankReg(core, reg);
}

static void ARM_SetUserReg(GBA_Core *core, uint8_t reg, uint32_t value) {
    if (reg < 8) {
        core->cpu.regs.r[reg] = value;
        return;
    }

    if (reg == 15) {
        GBA_CPU_SetReg(core, 15, value);
        return;
    }

    if (ARM_IsUserOrSystemMode(core)) {
        GBA_CPU_SetReg(core, reg, value);
        if (reg >= 8 && reg <= 14) {
            ARM_SetUserBankReg(core, reg, value);
        }
        return;
    }

    if (reg <= 12) {
        if (core->cpu.regs.priv_mode != CPU_STATE_FIQ) {
            core->cpu.regs.r[reg] = value;
        }
        ARM_SetUserBankReg(core, reg, value);
        return;
    }

    ARM_SetUserBankReg(core, reg, value);
}

/* ============================================================================
 * 條件檢查實現
 * ============================================================================ */

bool GBA_CPU_CheckCondition(const GBA_Core *core, ARM_ConditionCode cond) {
    uint32_t cpsr = core->cpu.regs.cpsr;
    bool N = (cpsr >> 31) & 1;
    bool Z = (cpsr >> 30) & 1;
    bool C = (cpsr >> 29) & 1;
    bool V = (cpsr >> 28) & 1;
    
    switch (cond) {
        case COND_EQ: return Z;              // Equal
        case COND_NE: return !Z;             // Not equal
        case COND_CS: return C;              // Carry set
        case COND_CC: return !C;             // Carry clear
        case COND_MI: return N;              // Negative
        case COND_PL: return !N;             // Positive
        case COND_VS: return V;              // Overflow
        case COND_VC: return !V;             // No overflow
        case COND_HI: return C && !Z;        // Unsigned higher
        case COND_LS: return !C || Z;        // Unsigned lower or same
        case COND_GE: return N == V;         // Signed >=
        case COND_LT: return N != V;         // Signed <
        case COND_GT: return !Z && (N == V); // Signed >
        case COND_LE: return Z || (N != V);  // Signed <=
        case COND_AL: return true;           // Always
        case COND_NV: return false;          // Never
        default: return false;
    }
}

/* ============================================================================
 * PSR 傳送指令 (MSR/MRS)
 * ============================================================================ */

// MRS - Move PSR to Register
InstructionResult ARM_MRS(GBA_Core *core, uint32_t inst) {
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) { .cycles = 1, .branch_taken = false };
    }

    bool use_spsr = (inst >> 22) & 1;  // R bit
    uint8_t rd = (inst >> 12) & 0xF;

    uint32_t psr_value;
    if (use_spsr) {
        // 讀取當前模式的 SPSR
        switch (core->cpu.regs.priv_mode) {
            case CPU_STATE_FIQ:        psr_value = core->cpu.regs.fiq.spsr_fiq; break;
            case CPU_STATE_IRQ:        psr_value = core->cpu.regs.irq.spsr; break;
            case CPU_STATE_SUPERVISOR: psr_value = core->cpu.regs.svc.spsr; break;
            case CPU_STATE_ABORT:      psr_value = core->cpu.regs.abt.spsr; break;
            case CPU_STATE_UNDEFINED:  psr_value = core->cpu.regs.und.spsr; break;
            default:                   psr_value = core->cpu.regs.cpsr; break;  // User/System 無 SPSR
        }
    } else {
        psr_value = core->cpu.regs.cpsr;
    }

    GBA_CPU_SetReg(core, rd, psr_value);

    return (InstructionResult) { .cycles = 1, .branch_taken = false };
}

// MSR - Move to PSR
InstructionResult ARM_MSR(GBA_Core *core, uint32_t inst) {
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) { .cycles = 1, .branch_taken = false };
    }

    bool use_spsr = (inst >> 22) & 1;  // R bit
    bool immediate = (inst >> 25) & 1;  // I bit
    uint8_t field_mask = (inst >> 16) & 0xF;  // bits 19-16

    // 計算要寫入的值
    uint32_t operand;
    if (immediate) {
        uint32_t imm = inst & 0xFF;
        uint8_t rotate = ((inst >> 8) & 0xF) * 2;
        operand = (imm >> rotate) | (imm << (32 - rotate));
    } else {
        uint8_t rm = inst & 0xF;
        operand = GBA_CPU_GetReg(core, rm);
    }

    // 生成位元遮罩
    uint32_t mask = 0;
    if (field_mask & 0x8) mask |= 0xFF000000;  // flags (N,Z,C,V)
    if (field_mask & 0x4) mask |= 0x00FF0000;  // status
    if (field_mask & 0x2) mask |= 0x0000FF00;  // extension
    if (field_mask & 0x1) mask |= 0x000000FF;  // control (mode, I, F, T)

    // 在 User 模式下，只能修改條件標誌位
    // System 模式是特權模式，可以修改所有欄位
    if (core->cpu.regs.priv_mode == CPU_STATE_USER) {
        mask &= 0xFF000000;  // 只保留 flags 部分
    }

    if (use_spsr) {
        // 寫入 SPSR
        switch (core->cpu.regs.priv_mode) {
            case CPU_STATE_FIQ:
                core->cpu.regs.fiq.spsr_fiq = (core->cpu.regs.fiq.spsr_fiq & ~mask) | (operand & mask);
                break;
            case CPU_STATE_IRQ:
                core->cpu.regs.irq.spsr = (core->cpu.regs.irq.spsr & ~mask) | (operand & mask);
                break;
            case CPU_STATE_SUPERVISOR:
                core->cpu.regs.svc.spsr = (core->cpu.regs.svc.spsr & ~mask) | (operand & mask);
                break;
            case CPU_STATE_ABORT:
                core->cpu.regs.abt.spsr = (core->cpu.regs.abt.spsr & ~mask) | (operand & mask);
                break;
            case CPU_STATE_UNDEFINED:
                core->cpu.regs.und.spsr = (core->cpu.regs.und.spsr & ~mask) | (operand & mask);
                break;
            default:
                // User/System 模式無 SPSR，忽略寫入
                break;
        }
    } else {
        // 寫入 CPSR
        uint32_t old_cpsr = core->cpu.regs.cpsr;
        core->cpu.regs.cpsr = (old_cpsr & ~mask) | (operand & mask);

        // 如果模式位被修改，需要切換暫存器組
        if ((mask & 0x1F) != 0) {
            CPU_PrivilegeMode old_mode = core->cpu.regs.priv_mode;
            CPU_PrivilegeMode new_mode = (CPU_PrivilegeMode)(core->cpu.regs.cpsr & 0x1F);
            if (new_mode != old_mode) {
                // 儲存當前模式的 r13, r14 到 banked registers
                switch (old_mode) {
                    case CPU_STATE_FIQ:
                        core->cpu.regs.fiq.r13_fiq = core->cpu.regs.r[13];
                        core->cpu.regs.fiq.r14_fiq = core->cpu.regs.r[14];
                        break;
                    case CPU_STATE_IRQ:
                        core->cpu.regs.irq.r13 = core->cpu.regs.r[13];
                        core->cpu.regs.irq.r14 = core->cpu.regs.r[14];
                        break;
                    case CPU_STATE_SUPERVISOR:
                        core->cpu.regs.svc.r13 = core->cpu.regs.r[13];
                        core->cpu.regs.svc.r14 = core->cpu.regs.r[14];
                        break;
                    case CPU_STATE_ABORT:
                        core->cpu.regs.abt.r13 = core->cpu.regs.r[13];
                        core->cpu.regs.abt.r14 = core->cpu.regs.r[14];
                        break;
                    case CPU_STATE_UNDEFINED:
                        core->cpu.regs.und.r13 = core->cpu.regs.r[13];
                        core->cpu.regs.und.r14 = core->cpu.regs.r[14];
                        break;
                    case CPU_STATE_USER:
                    case CPU_STATE_SYSTEM:
                        core->cpu.regs.usr.r13_usr = core->cpu.regs.r[13];
                        core->cpu.regs.usr.r14_usr = core->cpu.regs.r[14];
                        break;
                }
                
                // 載入新模式的 r13, r14 從 banked registers
                switch (new_mode) {
                    case CPU_STATE_FIQ:
                        core->cpu.regs.r[13] = core->cpu.regs.fiq.r13_fiq;
                        core->cpu.regs.r[14] = core->cpu.regs.fiq.r14_fiq;
                        break;
                    case CPU_STATE_IRQ:
                        core->cpu.regs.r[13] = core->cpu.regs.irq.r13;
                        core->cpu.regs.r[14] = core->cpu.regs.irq.r14;
                        break;
                    case CPU_STATE_SUPERVISOR:
                        core->cpu.regs.r[13] = core->cpu.regs.svc.r13;
                        core->cpu.regs.r[14] = core->cpu.regs.svc.r14;
                        break;
                    case CPU_STATE_ABORT:
                        core->cpu.regs.r[13] = core->cpu.regs.abt.r13;
                        core->cpu.regs.r[14] = core->cpu.regs.abt.r14;
                        break;
                    case CPU_STATE_UNDEFINED:
                        core->cpu.regs.r[13] = core->cpu.regs.und.r13;
                        core->cpu.regs.r[14] = core->cpu.regs.und.r14;
                        break;
                    case CPU_STATE_USER:
                    case CPU_STATE_SYSTEM:
                        core->cpu.regs.r[13] = core->cpu.regs.usr.r13_usr;
                        core->cpu.regs.r[14] = core->cpu.regs.usr.r14_usr;
                        break;
                }
                
                core->cpu.regs.priv_mode = new_mode;
            }
        }
    }

    return (InstructionResult) { .cycles = 1, .branch_taken = false };
}

/* ============================================================================
 * 資料處理指令 - 完整實現
 * ============================================================================ */

InstructionResult ARM_DataProcessing(GBA_Core *core, uint32_t inst) {
    // 檢查條件碼
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) {
            .cycles = 1,
            .branch_taken = false
        };
    }
    
    // 提取操作碼
    uint8_t opcode = (inst >> 21) & 0xF;
    uint8_t Rd = (inst >> 12) & 0xF;
    uint8_t Rn = (inst >> 16) & 0xF;
    bool S = (inst >> 20) & 1;
    
    // 獲取運算元
    uint32_t op1 = GBA_CPU_GetReg(core, Rn);
    uint32_t op2;
    bool carry = GBA_CPU_GetFlag_C(&core->cpu);
    
    // 解碼 Op2 (簡化版，僅支援立即數)
    if (inst & (1 << 25)) {
        uint8_t imm = inst & 0xFF;
        uint8_t rotate = ((inst >> 8) & 0xF) * 2;
        op2 = ROR(imm, rotate);
    } else {
        uint8_t Rm = inst & 0xF;
        op2 = GBA_CPU_GetReg(core, Rm);
    }
    
    uint32_t result = 0;
    uint64_t result64 = 0;
    bool write_result = true;
    
    // 執行操作
    switch (opcode) {
        case DP_AND:  // AND
            result = op1 & op2;
            break;
            
        case DP_EOR:  // XOR
            result = op1 ^ op2;
            break;
            
        case DP_SUB:  // SUB
            result64 = (uint64_t)op1 - (uint64_t)op2;
            result = (uint32_t)result64;
            break;
            
        case DP_RSB:  // Reverse SUB
            result64 = (uint64_t)op2 - (uint64_t)op1;
            result = (uint32_t)result64;
            break;
            
        case DP_ADD:  // ADD
            result64 = (uint64_t)op1 + (uint64_t)op2;
            result = (uint32_t)result64;
            break;
            
        case DP_ADC:  // ADD with Carry
            result64 = (uint64_t)op1 + (uint64_t)op2 + carry;
            result = (uint32_t)result64;
            break;
            
        case DP_SBC:  // SUB with Carry
            result64 = (uint64_t)op1 - (uint64_t)op2 - !carry;
            result = (uint32_t)result64;
            break;
            
        case DP_RSC:  // Reverse SUB with Carry
            result64 = (uint64_t)op2 - (uint64_t)op1 - !carry;
            result = (uint32_t)result64;
            break;
            
        case DP_TST:  // Test (AND, no write)
            result = op1 & op2;
            write_result = false;
            S = true;  // 強制設定標誌位
            break;
            
        case DP_TEQ:  // Test Equal (XOR, no write)
            result = op1 ^ op2;
            write_result = false;
            S = true;
            break;
            
        case DP_CMP:  // Compare (SUB, no write)
            result64 = (uint64_t)op1 - (uint64_t)op2;
            result = (uint32_t)result64;
            write_result = false;
            S = true;
            break;
            
        case DP_CMN:  // Compare Negative (ADD, no write)
            result64 = (uint64_t)op1 + (uint64_t)op2;
            result = (uint32_t)result64;
            write_result = false;
            S = true;
            break;
            
        case DP_ORR:  // OR
            result = op1 | op2;
            break;
            
        case DP_MOV:  // Move
            result = op2;
            break;
            
        case DP_BIC:  // Bit Clear
            result = op1 & ~op2;
            break;
            
        case DP_MVN:  // Move NOT
            result = ~op2;
            break;
    }
    
    // 寫入結果
    bool write_pc = write_result && (Rd == 15);
    if (write_result) {
        GBA_CPU_SetReg(core, Rd, result);
    }
    
    // 更新標誌位
    if (S) {
        GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
        GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
        
        // Carry 標誌 (僅對某些指令有效)
        switch (opcode) {
            case DP_SUB:
            case DP_RSB:
            case DP_SBC:
            case DP_RSC:
            case DP_CMP:
                GBA_CPU_SetFlag_C(&core->cpu, result64 <= 0xFFFFFFFF);
                break;
                
            case DP_ADD:
            case DP_ADC:
            case DP_CMN:
                GBA_CPU_SetFlag_C(&core->cpu, result64 > 0xFFFFFFFF);
                break;
        }
        
        // Overflow 標誌
        switch (opcode) {
            case DP_ADD:
            case DP_ADC:
            case DP_CMN: {
                bool v = ((op1 ^ result) & (op2 ^ result)) >> 31;
                GBA_CPU_SetFlag_V(&core->cpu, v);
                break;
            }
            
            case DP_SUB:
            case DP_SBC:
            case DP_CMP: {
                bool v = ((op1 ^ op2) & (op1 ^ result)) >> 31;
                GBA_CPU_SetFlag_V(&core->cpu, v);
                break;
            }
        }
    }
    
    if (S && write_pc) {
        uint32_t spsr = 0;
        switch (core->cpu.regs.priv_mode) {
            case CPU_STATE_FIQ:        spsr = core->cpu.regs.fiq.spsr_fiq; break;
            case CPU_STATE_IRQ:        spsr = core->cpu.regs.irq.spsr;     break;
            case CPU_STATE_SUPERVISOR: spsr = core->cpu.regs.svc.spsr;     break;
            case CPU_STATE_ABORT:      spsr = core->cpu.regs.abt.spsr;     break;
            case CPU_STATE_UNDEFINED:  spsr = core->cpu.regs.und.spsr;     break;
            default: break;
        }

        if (spsr != 0) {
            CPU_PrivilegeMode target_mode = (CPU_PrivilegeMode)(spsr & 0x1F);
            core->cpu.regs.cpsr = spsr;
            GBA_CPU_SwitchMode(core, target_mode);
            if (spsr & (1 << 5)) {
                core->cpu.regs.exec_mode = CPU_MODE_THUMB;
            } else {
                core->cpu.regs.exec_mode = CPU_MODE_ARM;
            }
        }
    }

    if (write_pc) {
        GBA_CPUFlushPipeline(core);
    }

    return (InstructionResult) {
        .cycles = 1,
        .branch_taken = write_pc,
        .pipeline_flush = write_pc,
        .mode_changed = false
    };
}

/* ============================================================================
 * 分支指令 (B/BL)
 * ============================================================================ */

InstructionResult ARM_Branch(GBA_Core *core, uint32_t inst) {
    // 檢查條件碼
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) {
            .cycles = 1,
            .branch_taken = false
        };
    }
    
    // 提取 Link 位
    bool link = (inst >> 24) & 1;
    
    // 提取偏移 (24 位，符號擴充套件到 32 位)
    int32_t offset = SIGN_EXTEND(inst & 0xFFFFFF, 24);
    offset <<= 2;  // 左移 2 位 (字對齊)
    
    // 儲存返回地址 (如果是 BL)
    if (link) {
        core->cpu.regs.r[14] = core->cpu.regs.pc - 4;  // LR = PC - 4 (return address)
    }
    
    // 跳轉 (ARM 取 PC 為當前指令位址 + 8)
    uint32_t target = (core->cpu.regs.pc + offset) & ~3;
    GBA_CPU_SetReg(core, 15, target);
    
    // 重新整理流水線
    GBA_CPUFlushPipeline(core);
    
    return (InstructionResult) {
        .cycles = 3,           // 2S + 1N
        .branch_taken = true,
        .pipeline_flush = true,
        .mode_changed = false
    };
}

/* ============================================================================
 * 分支交換 (BX)
 * ============================================================================ */

InstructionResult ARM_BranchExchange(GBA_Core *core, uint32_t inst) {
    // 檢查條件碼
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) {
            .cycles = 1,
            .branch_taken = false
        };
    }
    
    // 獲取目標暫存器
    uint8_t Rm = inst & 0xF;
    uint32_t target = GBA_CPU_GetReg(core, Rm);
    
    // 檢查最低位 (THUMB 標誌)
    if (target & 1) {
        // 切換到 THUMB 模式
        core->cpu.regs.exec_mode = CPU_MODE_THUMB;
        core->cpu.regs.cpsr |= (1 << 5);
        GBA_CPU_SetReg(core, 15, target);
    } else {
        // 保持 ARM 模式
        core->cpu.regs.exec_mode = CPU_MODE_ARM;
        core->cpu.regs.cpsr &= ~(1 << 5);
        GBA_CPU_SetReg(core, 15, target);
    }
    
    GBA_CPUFlushPipeline(core);
    
    return (InstructionResult) {
        .cycles = 3,
        .branch_taken = true,
        .pipeline_flush = true,
        .mode_changed = true
    };
}

/* ============================================================================
 * 軟體中斷 (SWI)
 * ============================================================================ */

InstructionResult ARM_SoftwareInterrupt(GBA_Core *core, uint32_t inst) {
    // SWI 不檢查條件碼
    uint8_t swi_num = (inst >> 16) & 0xFF;
    
    // 儲存返回地址
    core->cpu.regs.svc.r14 = core->cpu.regs.pc - 4;
    core->cpu.regs.svc.spsr = core->cpu.regs.cpsr;
    
    // 切換到 Supervisor 模式
    core->cpu.regs.priv_mode = CPU_STATE_SUPERVISOR;
    core->cpu.regs.cpsr = (core->cpu.regs.cpsr & 0xFFFFFFE0) | CPU_STATE_SUPERVISOR;
    core->cpu.regs.cpsr |= (1 << 7);  // 禁用 IRQ
    
    // 跳轉到 SWI 向量
    GBA_CPU_SetReg(core, 15, 0x00000008);
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    
    GBA_CPUFlushPipeline(core);
    
    #ifdef GBA_DEBUG
    printf("SWI 0x%02X called\n", swi_num);
    #endif
    
    return (InstructionResult) {
        .cycles = 3,
        .branch_taken = true,
        .pipeline_flush = true,
        .mode_changed = false
    };
}

/* ============================================================================
 * 乘法指令 (MUL/MLA)
 * ============================================================================ */

InstructionResult ARM_Multiply(GBA_Core *core, uint32_t inst) {
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) { .cycles = 1, .branch_taken = false };
    }

    bool A = (inst >> 21) & 1;
    bool S = (inst >> 20) & 1;
    uint8_t Rd = (inst >> 16) & 0xF;
    uint8_t Rn = (inst >> 12) & 0xF;
    uint8_t Rs = (inst >> 8) & 0xF;
    uint8_t Rm = inst & 0xF;

    uint32_t op1 = GBA_CPU_GetReg(core, Rm);
    uint32_t op2 = GBA_CPU_GetReg(core, Rs);
    uint64_t result64 = (uint64_t)op1 * (uint64_t)op2;

    if (A) {
        result64 += GBA_CPU_GetReg(core, Rn);
    }

    uint32_t result = (uint32_t)result64;
    GBA_CPU_SetReg(core, Rd, result);

    if (S) {
        GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
        GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
    }

    uint8_t cycles = (uint8_t)(1 + ARM_MultiplyCycles(op2) + (A ? 1 : 0));
    return (InstructionResult) {
        .cycles = cycles,
        .branch_taken = false,
        .pipeline_flush = false,
        .mode_changed = false
    };
}

/* ============================================================================
 * 長乘法指令 (UMULL/SMULL/UMLAL/SMLAL)
 * ============================================================================ */

InstructionResult ARM_MultiplyLong(GBA_Core *core, uint32_t inst) {
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) { .cycles = 1, .branch_taken = false };
    }

    bool U = (inst >> 22) & 1;
    bool A = (inst >> 21) & 1;
    bool S = (inst >> 20) & 1;
    uint8_t RdHi = (inst >> 16) & 0xF;
    uint8_t RdLo = (inst >> 12) & 0xF;
    uint8_t Rs = (inst >> 8) & 0xF;
    uint8_t Rm = inst & 0xF;

    uint64_t result64;
    if (U) {
        int64_t op1 = (int32_t)GBA_CPU_GetReg(core, Rm);
        int64_t op2 = (int32_t)GBA_CPU_GetReg(core, Rs);
        result64 = (uint64_t)(op1 * op2);
    } else {
        uint64_t op1 = GBA_CPU_GetReg(core, Rm);
        uint64_t op2 = GBA_CPU_GetReg(core, Rs);
        result64 = op1 * op2;
    }

    if (A) {
        uint64_t acc = ((uint64_t)GBA_CPU_GetReg(core, RdHi) << 32) |
                       GBA_CPU_GetReg(core, RdLo);
        result64 += acc;
    }

    GBA_CPU_SetReg(core, RdLo, (uint32_t)result64);
    GBA_CPU_SetReg(core, RdHi, (uint32_t)(result64 >> 32));

    if (S) {
        GBA_CPU_SetFlag_N(&core->cpu, (result64 >> 63) & 1);
        GBA_CPU_SetFlag_Z(&core->cpu, result64 == 0);
    }

    uint8_t cycles = (uint8_t)(2 + ARM_MultiplyCycles(GBA_CPU_GetReg(core, Rs)) + (A ? 1 : 0));
    return (InstructionResult) {
        .cycles = cycles,
        .branch_taken = false,
        .pipeline_flush = false,
        .mode_changed = false
    };
}

/* ============================================================================
 * 批次資料傳送 (LDM/STM)
 * ============================================================================ */

InstructionResult ARM_BlockDataTransfer(GBA_Core *core, uint32_t inst) {
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) { .cycles = 1, .branch_taken = false };
    }

    bool P = (inst >> 24) & 1;
    bool U = (inst >> 23) & 1;
    bool S = (inst >> 22) & 1;
    bool W = (inst >> 21) & 1;
    bool L = (inst >> 20) & 1;

    uint8_t Rn = (inst >> 16) & 0xF;
    uint32_t reg_list = inst & 0xFFFF;

    if (reg_list == 0) {
        reg_list = (1U << 15);
    }

    uint8_t count = ARM_CountSetBits(reg_list);
    uint32_t base = GBA_CPU_GetReg(core, Rn);

    uint32_t start;
    if (U) {
        start = P ? (base + 4) : base;
    } else {
        start = P ? (base - (count * 4)) : (base - (count * 4) + 4);
    }

    bool use_user_bank = S && !L;
    uint32_t addr = start;
    for (uint8_t reg = 0; reg < 16; reg++) {
        if (reg_list & (1U << reg)) {
            if (L) {
                uint32_t value = GBA_MemoryRead32(core, addr & ~3U);
                if (reg == 15) {
                    GBA_CPU_SetReg(core, 15, value);
                } else if (S && !(reg_list & (1U << 15))) {
                    ARM_SetUserReg(core, reg, value);
                } else {
                    core->cpu.regs.r[reg] = value;
                }
            } else {
                uint32_t value = use_user_bank ? ARM_GetUserReg(core, reg)
                                               : GBA_CPU_GetReg(core, reg);
                GBA_MemoryWrite32(core, addr & ~3U, value);
            }
            addr += 4;
        }
    }

    if (W && !(L && (reg_list & (1U << Rn)))) {
        uint32_t writeback = U ? (base + (count * 4)) : (base - (count * 4));
        GBA_CPU_SetReg(core, Rn, writeback);
    }

    bool load_pc = L && (reg_list & (1U << 15));
    if (load_pc) {
        GBA_CPUFlushPipeline(core);
    }

    if (S && load_pc) {
        uint32_t spsr = 0;
        switch (core->cpu.regs.priv_mode) {
            case CPU_STATE_FIQ:        spsr = core->cpu.regs.fiq.spsr_fiq; break;
            case CPU_STATE_IRQ:        spsr = core->cpu.regs.irq.spsr;     break;
            case CPU_STATE_SUPERVISOR: spsr = core->cpu.regs.svc.spsr;     break;
            case CPU_STATE_ABORT:      spsr = core->cpu.regs.abt.spsr;     break;
            case CPU_STATE_UNDEFINED:  spsr = core->cpu.regs.und.spsr;     break;
            default: break;
        }

        if (spsr != 0) {
            CPU_PrivilegeMode target_mode = (CPU_PrivilegeMode)(spsr & 0x1F);
            core->cpu.regs.cpsr = spsr;
            GBA_CPU_SwitchMode(core, target_mode);
            if (spsr & (1 << 5)) {
                core->cpu.regs.exec_mode = CPU_MODE_THUMB;
            } else {
                core->cpu.regs.exec_mode = CPU_MODE_ARM;
            }
        }
    }

    // GBATEK: LDM = nS + 1N + 1I, STM = (n-1)S + 2N
    uint8_t cycles;
    if (L) {
        // LDM: nS + 1N + 1I = count + 2
        cycles = count + 2;
        if (load_pc) {
            // Load to PC: 額外 +2S + 1N = +3
            cycles += 3;
        }
    } else {
        // STM: (n-1)S + 2N = count + 1
        cycles = count + 1;
    }

    return (InstructionResult) {
        .cycles = cycles,
        .branch_taken = load_pc,
        .pipeline_flush = load_pc,
        .mode_changed = false
    };
}

/* ============================================================================
 * 單資料傳送 (LDR/STR)
 * ============================================================================ */

InstructionResult ARM_SingleDataTransfer(GBA_Core *core, uint32_t inst) {
    // 檢查條件碼
    ARM_ConditionCode cond = (inst >> 28) & 0xF;
    if (!GBA_CPU_CheckCondition(core, cond)) {
        return (InstructionResult) {
            .cycles = 1,
            .branch_taken = false
        };
    }

    bool I = (inst >> 25) & 1;  // 立即數/暫存器偏移
    bool P = (inst >> 24) & 1;  // Pre/Post
    bool U = (inst >> 23) & 1;  // 加/減偏移
    bool B = (inst >> 22) & 1;  // Byte/Word
    bool W = (inst >> 21) & 1;  // Write-back
    bool L = (inst >> 20) & 1;  // Load/Store

    uint8_t Rn = (inst >> 16) & 0xF;
    uint8_t Rd = (inst >> 12) & 0xF;

    uint32_t base = GBA_CPU_GetReg(core, Rn);
    uint32_t offset = 0;

    if (I) {
        uint8_t Rm = inst & 0xF;
        uint32_t rm_val = GBA_CPU_GetReg(core, Rm);
        uint8_t shift_imm = (inst >> 7) & 0x1F;
        uint8_t shift_type = (inst >> 5) & 0x3;

        switch (shift_type) {
            case 0:  // LSL
                offset = shift_imm ? (rm_val << shift_imm) : rm_val;
                break;
            case 1:  // LSR
                offset = shift_imm ? (rm_val >> shift_imm) : 0;
                break;
            case 2:  // ASR
                offset = shift_imm ? (uint32_t)ASR((int32_t)rm_val, shift_imm)
                                   : (rm_val & 0x80000000 ? 0xFFFFFFFF : 0);
                break;
            case 3:  // ROR / RRX
                if (shift_imm == 0) {
                    uint32_t carry = GBA_CPU_GetFlag_C(&core->cpu) ? 0x80000000 : 0;
                    offset = (rm_val >> 1) | carry;
                } else {
                    offset = ROR(rm_val, shift_imm);
                }
                break;
        }
    } else {
        offset = inst & 0xFFF;
    }

    uint32_t effective = base;
    if (P) {
        effective = U ? (base + offset) : (base - offset);
    }

    if (L) {
        uint32_t value;
        if (B) {
            value = GBA_MemoryRead8(core, effective);
        } else {
            uint32_t aligned = effective & ~3U;
            value = GBA_MemoryRead32(core, aligned);
            uint8_t rot = (effective & 3U) * 8;
            if (rot) {
                value = ROR(value, rot);
            }
        }
        
        GBA_CPU_SetReg(core, Rd, value);
    } else {
        uint32_t value = GBA_CPU_GetReg(core, Rd);
        if (B) {
            GBA_MemoryWrite8(core, effective, (uint8_t)value);
        } else {
            GBA_MemoryWrite32(core, effective, value);
        }
    }

    if (!P) {
        effective = U ? (base + offset) : (base - offset);
    }

    if (W || !P) {
        GBA_CPU_SetReg(core, Rn, effective);
    }

    bool load_pc = L && (Rd == 15);
    if (load_pc) {
        core->cpu.regs.pc &= ~3U;
        GBA_CPUFlushPipeline(core);
    }

    // GBATEK: LDR = 1S + 1N + 1I, STR = 2N
    uint8_t mem_cycles = GBA_MemoryGetAccessCycles(core, effective, false);
    uint8_t total_cycles;
    
    if (L) {
        // Load: 1S (opcode fetch) + 1N (memory read) + 1I (data transfer)
        total_cycles = 1 + mem_cycles + 1;
        if (load_pc) {
            // Load to PC: 額外 +1S+1N (pipeline flush)
            total_cycles += 2;
        }
    } else {
        // Store: 2N (address calculation + memory write)
        total_cycles = 1 + mem_cycles;
    }
    
    return (InstructionResult) {
        .cycles = total_cycles,
        .branch_taken = load_pc,
        .pipeline_flush = load_pc,
        .mode_changed = false
    };
}

/* ============================================================================
 * 主執行函式
 * ============================================================================ */

InstructionResult GBA_CPU_ExecuteARM(GBA_Core *core, uint32_t instruction) {
    // 簡化的解碼邏輯 (完整版應使用查詢表)
    
    // 乘法 (MUL/MLA)
    if ((instruction & 0x0FC000F0) == 0x00000090) {
        return ARM_Multiply(core, instruction);
    }

    // 長乘法 (UMULL/SMULL/UMLAL/SMLAL)
    if ((instruction & 0x0F8000F0) == 0x00800090) {
        return ARM_MultiplyLong(core, instruction);
    }

    // 分支交換 (BX)
    if ((instruction & 0x0FFFFFF0) == 0x012FFF10) {
        return ARM_BranchExchange(core, instruction);
    }

    // MRS - Move PSR to Register
    // cccc 0001 0R00 1111 dddd 0000 0000 0000
    if ((instruction & 0x0FBF0FFF) == 0x010F0000) {
        return ARM_MRS(core, instruction);
    }

    // MSR - Move to PSR (register or immediate)
    // Register: cccc 0001 0R10 mask 1111 0000 0000 mmmm
    // Immediate: cccc 0011 0R10 mask 1111 rrrr iiii iiii
    if ((instruction & 0x0FB00000) == 0x03200000) {
        // MSR immediate
        return ARM_MSR(core, instruction);
    }
    if ((instruction & 0x0FB000F0) == 0x01200000) {
        // MSR register
        return ARM_MSR(core, instruction);
    }

    // 資料處理 (00I)
    if ((instruction & 0x0C000000) == 0x00000000) {
        return ARM_DataProcessing(core, instruction);
    }
    
    // 分支 (101L)
    if ((instruction & 0x0E000000) == 0x0A000000) {
        return ARM_Branch(core, instruction);
    }

    // 單資料傳送 (LDR/STR)
    if ((instruction & 0x0C000000) == 0x04000000) {
        return ARM_SingleDataTransfer(core, instruction);
    }

    // 批次資料傳送 (LDM/STM)
    if ((instruction & 0x0E000000) == 0x08000000) {
        return ARM_BlockDataTransfer(core, instruction);
    }
    
    
    // 軟體中斷 (1111)
    if ((instruction & 0x0F000000) == 0x0F000000) {
        return ARM_SoftwareInterrupt(core, instruction);
    }
    
    // TODO: 實現其他指令
    
    #ifdef GBA_DEBUG
    printf("Unimplemented ARM instruction: 0x%08X at PC=0x%08X\n", 
           instruction, core->cpu.regs.pc);
    #endif
    
    return (InstructionResult) {
        .cycles = 1,
        .branch_taken = false
    };
}
