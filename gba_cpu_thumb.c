/**
 * THUMB 指令集完整實現
 * THUMB 是 ARM 的 16 位指令子集
 */

#include "gba_cpu_instructions.h"
#include "gba_core.h"
#include <stdio.h>

static uint8_t THUMB_CountSetBits(uint32_t value) {
    uint8_t count = 0;
    while (value) {
        count += (value & 1U);
        value >>= 1;
    }
    return count;
}

/* ============================================================================
 * THUMB 資料處理指令
 * ============================================================================ */

InstructionResult THUMB_MoveShiftedRegister(GBA_Core *core, uint16_t inst) {
    uint8_t opcode = (inst >> 11) & 0x3;
    uint8_t offset = (inst >> 6) & 0x1F;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    
    uint32_t value = core->cpu.regs.r[Rs];
    uint32_t result = 0;
    bool carry = GBA_CPU_GetFlag_C(&core->cpu);
    
    switch (opcode) {
        case 0:  // LSL
            if (offset == 0) {
                result = value;
            } else {
                carry = (value >> (32 - offset)) & 1;
                result = value << offset;
            }
            break;
            
        case 1:  // LSR
            if (offset == 0) offset = 32;
            carry = (value >> (offset - 1)) & 1;
            result = value >> offset;
            break;
            
        case 2:  // ASR
            if (offset == 0) offset = 32;
            carry = (value >> (offset - 1)) & 1;
            result = (int32_t)value >> offset;
            break;
    }
    
    core->cpu.regs.r[Rd] = result;
    
    // 更新標誌位
    GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
    GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
    GBA_CPU_SetFlag_C(&core->cpu, carry);
    
    return (InstructionResult) {
        .cycles = 1,
        .branch_taken = false
    };
}

InstructionResult THUMB_AddSubtract(GBA_Core *core, uint16_t inst) {
    bool is_immediate = (inst >> 10) & 1;
    bool is_subtract = (inst >> 9) & 1;
    uint8_t Rn_offset = (inst >> 6) & 0x7;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    
    uint32_t op1 = core->cpu.regs.r[Rs];
    uint32_t op2 = is_immediate ? Rn_offset : core->cpu.regs.r[Rn_offset];
    
    uint32_t result;
    uint64_t result64;
    
    if (is_subtract) {
        result64 = (uint64_t)op1 - (uint64_t)op2;
        result = (uint32_t)result64;
        
        GBA_CPU_SetFlag_C(&core->cpu, op1 >= op2);
        GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ op2) & (op1 ^ result)) >> 31);
    } else {
        result64 = (uint64_t)op1 + (uint64_t)op2;
        result = (uint32_t)result64;
        
        GBA_CPU_SetFlag_C(&core->cpu, result64 > 0xFFFFFFFF);
        GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ result) & (op2 ^ result)) >> 31);
    }
    
    core->cpu.regs.r[Rd] = result;
    
    GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
    GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
    
    return (InstructionResult) {
        .cycles = 1,
        .branch_taken = false
    };
}

InstructionResult THUMB_MoveCmpAddSubImm(GBA_Core *core, uint16_t inst) {
    uint8_t opcode = (inst >> 11) & 0x3;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint8_t offset = inst & 0xFF;
    
    uint32_t op1 = core->cpu.regs.r[Rd];
    uint32_t result;
    uint64_t result64;
    
    switch (opcode) {
        case 0:  // MOV
            result = offset;
            core->cpu.regs.r[Rd] = result;
            GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
            GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
            break;
            
        case 1:  // CMP
            result64 = (uint64_t)op1 - (uint64_t)offset;
            result = (uint32_t)result64;
            GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
            GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
            GBA_CPU_SetFlag_C(&core->cpu, op1 >= offset);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ offset) & (op1 ^ result)) >> 31);
            break;
            
        case 2:  // ADD
            result64 = (uint64_t)op1 + (uint64_t)offset;
            result = (uint32_t)result64;
            core->cpu.regs.r[Rd] = result;
            GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
            GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
            GBA_CPU_SetFlag_C(&core->cpu, result64 > 0xFFFFFFFF);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ result) & (offset ^ result)) >> 31);
            break;
            
        case 3:  // SUB
            result64 = (uint64_t)op1 - (uint64_t)offset;
            result = (uint32_t)result64;
            core->cpu.regs.r[Rd] = result;
            GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
            GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
            GBA_CPU_SetFlag_C(&core->cpu, op1 >= offset);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ offset) & (op1 ^ result)) >> 31);
            break;
    }
    
    return (InstructionResult) {
        .cycles = 1,
        .branch_taken = false
    };
}

/* ============================================================================
 * THUMB 分支指令
 * ============================================================================ */

InstructionResult THUMB_ConditionalBranch(GBA_Core *core, uint16_t inst) {
    // THUMB 條件分支格式：Bits[15:12]=1101, Bits[11:8]=condition, Bits[7:0]=offset(8-bit signed)
    uint8_t cond = (inst >> 8) & 0xF;
    int8_t offset = inst & 0xFF;
    
    // 檢查條件
    if (!GBA_CPU_CheckCondition(core, (ARM_ConditionCode)cond)) {
        return (InstructionResult) {
            .cycles = 1,
            .branch_taken = false
        };
    }
    
    // 符號擴充套件並左移 1 位（offset 是 8 位有符號）
    int32_t offset32 = (int32_t)offset << 1;
    
    // 計算跳轉目標
    // 分支指令跳轉到: pc  + offset
    // (pc 已經是 exec_addr + 4，所以這裡已經考慮了pipeline)
    uint32_t target_addr = core->cpu.regs.pc + offset32;
    
    // 更新 exec_addr 和 pc
    core->cpu.regs.exec_addr = target_addr;
    core->cpu.regs.pc = target_addr + 4;  // THUMB: pc = exec_addr + 4
    GBA_CPUFlushPipeline(core);
    
    return (InstructionResult) {
        .cycles = 3,
        .branch_taken = true,
        .pipeline_flush = true
    };
}

InstructionResult THUMB_UnconditionalBranch(GBA_Core *core, uint16_t inst) {
    int16_t offset = inst & 0x7FF;
    
    // 符號擴充套件
    if (offset & 0x400) {
        offset |= 0xF800;
    }
    
    // 左移 1 位
    int32_t offset32 = (int32_t)offset << 1;
    
    // 計算跳轉目標
    uint32_t target_addr = core->cpu.regs.pc + offset32;
    
    // 更新 exec_addr 和 pc
    core->cpu.regs.exec_addr = target_addr;
    core->cpu.regs.pc = target_addr + 4;  // THUMB
    GBA_CPUFlushPipeline(core);
    
    return (InstructionResult) {
        .cycles = 3,
        .branch_taken = true,
        .pipeline_flush = true
    };
}

InstructionResult THUMB_LongBranchLink(GBA_Core *core, uint16_t inst) {
    bool is_second_instruction = (inst >> 11) & 1;
    uint16_t offset = inst & 0x7FF;
    
    if (!is_second_instruction) {
        // 第一條指令：設定 LR 高位
        int32_t offset32 = (int32_t)(offset << 21) >> 9;  // 符號擴充套件並左移 12 位
        core->cpu.regs.r[14] = core->cpu.regs.pc + offset32;
        
        return (InstructionResult) {
            .cycles = 1,
            .branch_taken = false
        };
    } else {
        // 第二條指令：跳轉
        uint32_t temp = core->cpu.regs.pc - 2;
        core->cpu.regs.pc = (core->cpu.regs.r[14] + (offset << 1)) & ~1;
        core->cpu.regs.r[14] = temp | 1;  // 設定 THUMB 位
        
        GBA_CPUFlushPipeline(core);
        
        return (InstructionResult) {
            .cycles = 3,
            .branch_taken = true,
            .pipeline_flush = true
        };
    }
}

/* ============================================================================
 * THUMB 載入/儲存指令（佔位符實現）
 * ============================================================================ */

InstructionResult THUMB_ALUOperation(GBA_Core *core, uint16_t inst) {
    uint8_t opcode = (inst >> 6) & 0xF;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;

    uint32_t op1 = core->cpu.regs.r[Rd];
    uint32_t op2 = core->cpu.regs.r[Rs];
    uint32_t result = 0;
    uint64_t result64 = 0;
    bool write_result = true;
    bool carry = GBA_CPU_GetFlag_C(&core->cpu);

    switch (opcode) {
        case 0x0:  // AND
            result = op1 & op2;
            break;
        case 0x1:  // EOR
            result = op1 ^ op2;
            break;
        case 0x2:  // LSL (register)
            if ((op2 & 0xFF) == 0) {
                result = op1;
            } else if ((op2 & 0xFF) < 32) {
                carry = (op1 >> (32 - (op2 & 0xFF))) & 1;
                result = op1 << (op2 & 0xFF);
            } else if ((op2 & 0xFF) == 32) {
                carry = op1 & 1;
                result = 0;
            } else {
                carry = 0;
                result = 0;
            }
            break;
        case 0x3:  // LSR (register)
            if ((op2 & 0xFF) == 0) {
                result = op1;
            } else if ((op2 & 0xFF) < 32) {
                carry = (op1 >> ((op2 & 0xFF) - 1)) & 1;
                result = op1 >> (op2 & 0xFF);
            } else if ((op2 & 0xFF) == 32) {
                carry = (op1 >> 31) & 1;
                result = 0;
            } else {
                carry = 0;
                result = 0;
            }
            break;
        case 0x4:  // ASR (register)
            if ((op2 & 0xFF) == 0) {
                result = op1;
            } else if ((op2 & 0xFF) < 32) {
                carry = (op1 >> ((op2 & 0xFF) - 1)) & 1;
                result = (uint32_t)ASR((int32_t)op1, op2 & 0xFF);
            } else {
                carry = (op1 >> 31) & 1;
                result = (op1 & 0x80000000) ? 0xFFFFFFFF : 0;
            }
            break;
        case 0x5:  // ADC
            result64 = (uint64_t)op1 + (uint64_t)op2 + (carry ? 1 : 0);
            result = (uint32_t)result64;
            GBA_CPU_SetFlag_C(&core->cpu, result64 > 0xFFFFFFFF);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ result) & (op2 ^ result)) >> 31);
            break;
        case 0x6:  // SBC
            result64 = (uint64_t)op1 - (uint64_t)op2 - (carry ? 0 : 1);
            result = (uint32_t)result64;
            GBA_CPU_SetFlag_C(&core->cpu, result64 <= 0xFFFFFFFF);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ op2) & (op1 ^ result)) >> 31);
            break;
        case 0x7:  // ROR
            if ((op2 & 0xFF) == 0) {
                result = op1;
            } else {
                uint8_t rot = (uint8_t)(op2 & 0x1F);
                result = ROR(op1, rot);
                carry = (result >> 31) & 1;
            }
            break;
        case 0x8:  // TST
            result = op1 & op2;
            write_result = false;
            break;
        case 0x9:  // NEG
            result64 = (uint64_t)0 - (uint64_t)op2;
            result = (uint32_t)result64;
            GBA_CPU_SetFlag_C(&core->cpu, result64 <= 0xFFFFFFFF);
            GBA_CPU_SetFlag_V(&core->cpu, ((0 ^ op2) & (0 ^ result)) >> 31);
            break;
        case 0xA:  // CMP
            result64 = (uint64_t)op1 - (uint64_t)op2;
            result = (uint32_t)result64;
            write_result = false;
            GBA_CPU_SetFlag_C(&core->cpu, result64 <= 0xFFFFFFFF);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ op2) & (op1 ^ result)) >> 31);
            break;
        case 0xB:  // CMN
            result64 = (uint64_t)op1 + (uint64_t)op2;
            result = (uint32_t)result64;
            write_result = false;
            GBA_CPU_SetFlag_C(&core->cpu, result64 > 0xFFFFFFFF);
            GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ result) & (op2 ^ result)) >> 31);
            break;
        case 0xC:  // ORR
            result = op1 | op2;
            break;
        case 0xD:  // MUL
            result = op1 * op2;
            break;
        case 0xE:  // BIC
            result = op1 & ~op2;
            break;
        case 0xF:  // MVN
            result = ~op2;
            break;
    }

    if (write_result) {
        core->cpu.regs.r[Rd] = result;
    }

    GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
    GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
    if (opcode == 0x2 || opcode == 0x3 || opcode == 0x4 || opcode == 0x7) {
        GBA_CPU_SetFlag_C(&core->cpu, carry);
    }

    return (InstructionResult) { .cycles = 1, .branch_taken = false };
}

InstructionResult THUMB_HiRegisterOps(GBA_Core *core, uint16_t inst) {
    uint8_t opcode = (inst >> 8) & 0x3;
    bool h1 = (inst >> 7) & 1;
    bool h2 = (inst >> 6) & 1;
    uint8_t Rs = ((inst >> 3) & 0x7) | (h2 << 3);
    uint8_t Rd = (inst & 0x7) | (h1 << 3);
    
    if (opcode == 3) {
        // BX
        uint32_t target = GBA_CPU_GetReg(core, Rs);
        
        if (target & 1) {
            // 保持 THUMB 模式
            core->cpu.regs.pc = target & ~1;
        } else {
            // 切換到 ARM 模式
            core->cpu.regs.exec_mode = CPU_MODE_ARM;
            core->cpu.regs.cpsr &= ~(1 << 5);
            core->cpu.regs.pc = target & ~3;
        }
        
        GBA_CPUFlushPipeline(core);
        
        return (InstructionResult) {
            .cycles = 3,
            .branch_taken = true,
            .pipeline_flush = true,
            .mode_changed = true
        };
    }

    uint32_t op1 = GBA_CPU_GetReg(core, Rd);
    uint32_t op2 = GBA_CPU_GetReg(core, Rs);

    if (opcode == 0) {  // ADD
        uint32_t result = op1 + op2;
        GBA_CPU_SetReg(core, Rd, result);
        if (Rd == 15) {
            GBA_CPUFlushPipeline(core);
            return (InstructionResult) { .cycles = 3, .branch_taken = true, .pipeline_flush = true };
        }
    } else if (opcode == 1) {  // CMP
        uint32_t result = op1 - op2;
        GBA_CPU_SetFlag_N(&core->cpu, result >> 31);
        GBA_CPU_SetFlag_Z(&core->cpu, result == 0);
        GBA_CPU_SetFlag_C(&core->cpu, op1 >= op2);
        GBA_CPU_SetFlag_V(&core->cpu, ((op1 ^ op2) & (op1 ^ result)) >> 31);
    } else if (opcode == 2) {  // MOV
        GBA_CPU_SetReg(core, Rd, op2);
        if (Rd == 15) {
            GBA_CPUFlushPipeline(core);
            return (InstructionResult) { .cycles = 3, .branch_taken = true, .pipeline_flush = true };
        }
    }

    return (InstructionResult) { .cycles = 1, .branch_taken = false };
}

InstructionResult THUMB_PCRelativeLoad(GBA_Core *core, uint16_t inst) {
    uint8_t Rd = (inst >> 8) & 0x7;
    uint32_t imm = (inst & 0xFF) << 2;
    // 在 THUMB 模式，pc 已經是 exec_addr + 4，不需要再加 4
    // ARM ARM spec: "address = (PC & ~2) + (offset << 2)"
    uint32_t pc = core->cpu.regs.pc & ~3U;
    uint32_t addr = pc + imm;

    uint32_t value = GBA_MemoryRead32(core, addr & ~3U);
    GBA_CPU_SetReg(core, Rd, value);

    return (InstructionResult) { .cycles = 3, .branch_taken = false };
}

InstructionResult THUMB_LoadStoreRegOffset(GBA_Core *core, uint16_t inst) {
    bool B = (inst >> 10) & 1;
    bool L = (inst >> 9) & 1;
    uint8_t Rm = (inst >> 6) & 0x7;
    uint8_t Rn = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;

    uint32_t addr = core->cpu.regs.r[Rn] + core->cpu.regs.r[Rm];

    if (L) {
        uint32_t value;
        if (B) {
            value = GBA_MemoryRead8(core, addr);
        } else {
            uint32_t aligned = addr & ~3U;
            value = GBA_MemoryRead32(core, aligned);
            uint8_t rot = (addr & 3U) * 8;
            if (rot) {
                value = ROR(value, rot);
            }
        }
        core->cpu.regs.r[Rd] = value;
    } else {
        uint32_t value = core->cpu.regs.r[Rd];
        if (B) {
            GBA_MemoryWrite8(core, addr, (uint8_t)value);
        } else {
            GBA_MemoryWrite32(core, addr & ~3U, value);
        }
    }

    // GBATEK: LDR = 1S + 1N + 1I (3), STR = 2N (2)
    return (InstructionResult) { .cycles = (uint8_t)(L ? 3 : 2), .branch_taken = false };
}

InstructionResult THUMB_LoadStoreSigned(GBA_Core *core, uint16_t inst) {
    bool H = (inst >> 10) & 1;
    bool S = (inst >> 9) & 1;
    uint8_t Rm = (inst >> 6) & 0x7;
    uint8_t Rn = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;

    uint32_t addr = core->cpu.regs.r[Rn] + core->cpu.regs.r[Rm];
    bool is_load = true;

    if (!H && !S) {  // STRH
        uint16_t value = (uint16_t)core->cpu.regs.r[Rd];
        GBA_MemoryWrite16(core, addr & ~1U, value);
        is_load = false;
    } else if (!H && S) {  // LDRH
        uint16_t value = GBA_MemoryRead16(core, addr & ~1U);
        core->cpu.regs.r[Rd] = value;
    } else if (H && !S) {  // LDSB
        int8_t value = (int8_t)GBA_MemoryRead8(core, addr);
        core->cpu.regs.r[Rd] = (int32_t)value;
    } else {  // LDSH
        int16_t value = (int16_t)GBA_MemoryRead16(core, addr & ~1U);
        core->cpu.regs.r[Rd] = (int32_t)value;
    }

    // GBATEK: LDR = 1S + 1N + 1I (3), STR = 2N (2)
    return (InstructionResult) { .cycles = (uint8_t)(is_load ? 3 : 2), .branch_taken = false };
}

InstructionResult THUMB_LoadStoreImmOffset(GBA_Core *core, uint16_t inst) {
    bool B = (inst >> 12) & 1;
    bool L = (inst >> 11) & 1;
    uint8_t imm5 = (inst >> 6) & 0x1F;
    uint8_t Rn = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;

    uint32_t offset = B ? imm5 : (imm5 << 2);
    uint32_t addr = core->cpu.regs.r[Rn] + offset;

    if (L) {
        uint32_t value;
        if (B) {
            value = GBA_MemoryRead8(core, addr);
        } else {
            uint32_t aligned = addr & ~3U;
            value = GBA_MemoryRead32(core, aligned);
            uint8_t rot = (addr & 3U) * 8;
            if (rot) {
                value = ROR(value, rot);
            }
        }
        core->cpu.regs.r[Rd] = value;
    } else {
        uint32_t value = core->cpu.regs.r[Rd];
        if (B) {
            GBA_MemoryWrite8(core, addr, (uint8_t)value);
        } else {
            GBA_MemoryWrite32(core, addr & ~3U, value);
        }
    }

    // GBATEK: LDR = 1S + 1N + 1I (3), STR = 2N (2)
    return (InstructionResult) { .cycles = (uint8_t)(L ? 3 : 2), .branch_taken = false };
}

InstructionResult THUMB_LoadStoreHalfword(GBA_Core *core, uint16_t inst) {
    bool L = (inst >> 11) & 1;
    uint8_t imm5 = (inst >> 6) & 0x1F;
    uint8_t Rn = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;

    uint32_t addr = core->cpu.regs.r[Rn] + (imm5 << 1);
    if (L) {
        uint16_t value = GBA_MemoryRead16(core, addr & ~1U);
        core->cpu.regs.r[Rd] = value;
    } else {
        uint16_t value = (uint16_t)core->cpu.regs.r[Rd];
        GBA_MemoryWrite16(core, addr & ~1U, value);
    }

    // GBATEK: LDR = 1S + 1N + 1I (3), STR = 2N (2)
    return (InstructionResult) { .cycles = (uint8_t)(L ? 3 : 2), .branch_taken = false };
}

InstructionResult THUMB_SPRelativeLoadStore(GBA_Core *core, uint16_t inst) {
    bool L = (inst >> 11) & 1;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint32_t imm = (inst & 0xFF) << 2;
    uint32_t addr = core->cpu.regs.r[13] + imm;

    if (L) {
        uint32_t value = GBA_MemoryRead32(core, addr & ~3U);
        core->cpu.regs.r[Rd] = value;
    } else {
        uint32_t value = core->cpu.regs.r[Rd];
        GBA_MemoryWrite32(core, addr & ~3U, value);
    }

    // GBATEK: LDR = 1S + 1N + 1I (3), STR = 2N (2)
    return (InstructionResult) { .cycles = (uint8_t)(L ? 3 : 2), .branch_taken = false };
}

InstructionResult THUMB_LoadAddress(GBA_Core *core, uint16_t inst) {
    bool use_sp = (inst >> 11) & 1;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint32_t imm = (inst & 0xFF) << 2;

    uint32_t base = use_sp ? core->cpu.regs.r[13] : ((core->cpu.regs.pc + 4) & ~3U);
    core->cpu.regs.r[Rd] = base + imm;

    return (InstructionResult) { .cycles = 1, .branch_taken = false };
}

InstructionResult THUMB_AddOffsetToSP(GBA_Core *core, uint16_t inst) {
    bool subtract = (inst >> 7) & 1;
    uint32_t imm = (inst & 0x7F) << 2;

    if (subtract) {
        core->cpu.regs.r[13] -= imm;
    } else {
        core->cpu.regs.r[13] += imm;
    }

    return (InstructionResult) { .cycles = 1, .branch_taken = false };
}

InstructionResult THUMB_PushPop(GBA_Core *core, uint16_t inst) {
    bool L = (inst >> 11) & 1;
    bool R = (inst >> 8) & 1;
    uint8_t reg_list = inst & 0xFF;
    uint8_t count = THUMB_CountSetBits(reg_list) + (R ? 1 : 0);
    uint32_t sp = core->cpu.regs.r[13];

    if (!L) {
        // PUSH: (n-1)S + 2N = count + 1
        uint32_t addr = sp - (count * 4);
        for (uint8_t reg = 0; reg < 8; reg++) {
            if (reg_list & (1U << reg)) {
                GBA_MemoryWrite32(core, addr, core->cpu.regs.r[reg]);
                addr += 4;
            }
        }
        if (R) {
            GBA_MemoryWrite32(core, addr, core->cpu.regs.r[14]);
        }
        core->cpu.regs.r[13] = sp - (count * 4);
        return (InstructionResult) { .cycles = (uint8_t)(count + 1), .branch_taken = false };
    }

    // POP: nS + 1N + 1I = count + 2
    uint32_t addr = sp;
    for (uint8_t reg = 0; reg < 8; reg++) {
        if (reg_list & (1U << reg)) {
            core->cpu.regs.r[reg] = GBA_MemoryRead32(core, addr & ~3U);
            addr += 4;
        }
    }
    
    uint8_t cycles = count + 2;
    if (R) {
        core->cpu.regs.pc = GBA_MemoryRead32(core, addr & ~3U) & ~1U;
        addr += 4;
        GBA_CPUFlushPipeline(core);
        // POP {PC}: 額外 +2S + 1N = +3
        cycles += 3;
    }

    core->cpu.regs.r[13] = sp + (count * 4);
    return (InstructionResult) {
        .cycles = cycles,
        .branch_taken = R,
        .pipeline_flush = R
    };
}

InstructionResult THUMB_MultipleLoadStore(GBA_Core *core, uint16_t inst) {
    bool L = (inst >> 11) & 1;
    uint8_t Rn = (inst >> 8) & 0x7;
    uint8_t reg_list = inst & 0xFF;
    uint8_t count = THUMB_CountSetBits(reg_list);

    uint32_t base = core->cpu.regs.r[Rn];
    uint32_t addr = base;

    if (reg_list == 0) {
        return (InstructionResult) { .cycles = 1, .branch_taken = false };
    }

    if (L) {
        // LDMIA: nS + 1N + 1I = count + 2
        for (uint8_t reg = 0; reg < 8; reg++) {
            if (reg_list & (1U << reg)) {
                core->cpu.regs.r[reg] = GBA_MemoryRead32(core, addr & ~3U);
                addr += 4;
            }
        }
    } else {
        // STMIA: (n-1)S + 2N = count + 1
        for (uint8_t reg = 0; reg < 8; reg++) {
            if (reg_list & (1U << reg)) {
                GBA_MemoryWrite32(core, addr & ~3U, core->cpu.regs.r[reg]);
                addr += 4;
            }
        }
    }

    core->cpu.regs.r[Rn] = base + (count * 4);
    return (InstructionResult) { .cycles = (uint8_t)(L ? (count + 2) : (count + 1)), .branch_taken = false };
}

InstructionResult THUMB_SoftwareInterrupt(GBA_Core *core, uint16_t inst) {
    uint8_t swi_num = inst & 0xFF;
    
    // 儲存返回地址
    core->cpu.regs.svc.r14 = core->cpu.regs.pc - 2;
    core->cpu.regs.svc.spsr = core->cpu.regs.cpsr;
    
    // 切換到 Supervisor 模式
    GBA_CPU_SwitchMode(core, CPU_STATE_SUPERVISOR);
    core->cpu.regs.cpsr |= (1 << 7);  // 禁用 IRQ
    
    // 切換到 ARM 模式
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.cpsr &= ~(1 << 5);
    
    // 跳轉到 SWI 向量
    core->cpu.regs.pc = 0x00000008;
    GBA_CPUFlushPipeline(core);
    
    #ifdef GBA_DEBUG
    printf("THUMB SWI 0x%02X called\n", swi_num);
    #endif
    
    return (InstructionResult) {
        .cycles = 3,
        .branch_taken = true,
        .pipeline_flush = true
    };
}

/* ============================================================================
 * 主執行函式
 * ============================================================================ */

InstructionResult GBA_CPU_ExecuteTHUMB(GBA_Core *core, uint16_t instruction) {
    // 簡化的解碼邏輯
    // 注意：更具體的掩碼（更多bits）應該先檢查
    
    // 加/減 (00011) - 更具體，先檢查
    if ((instruction & 0xF800) == 0x1800) {
        return THUMB_AddSubtract(core, instruction);
    }
    
    // 移位操作 (000xx) - 更通用，後檢查
    if ((instruction & 0xE000) == 0x0000) {
        return THUMB_MoveShiftedRegister(core, instruction);
    }
    
    // 立即數操作 (001xx)
    if ((instruction & 0xE000) == 0x2000) {
        return THUMB_MoveCmpAddSubImm(core, instruction);
    }

    // ALU 操作 (010000)
    if ((instruction & 0xFC00) == 0x4000) {
        return THUMB_ALUOperation(core, instruction);
    }
    
    // 高暫存器操作/BX (010001)
    if ((instruction & 0xFC00) == 0x4400) {
        return THUMB_HiRegisterOps(core, instruction);
    }

    // PC 相對載入 (01001)
    if ((instruction & 0xF800) == 0x4800) {
        return THUMB_PCRelativeLoad(core, instruction);
    }

    // 載入/儲存暫存器位移 (0101 0)
    if ((instruction & 0xF200) == 0x5000) {
        return THUMB_LoadStoreRegOffset(core, instruction);
    }

    // 載入/儲存有號/半字 (0101 1)
    if ((instruction & 0xF200) == 0x5200) {
        return THUMB_LoadStoreSigned(core, instruction);
    }

    // 載入/儲存立即數位移 (011)
    if ((instruction & 0xE000) == 0x6000) {
        return THUMB_LoadStoreImmOffset(core, instruction);
    }

    // 載入/儲存半字 (1000)
    if ((instruction & 0xF000) == 0x8000) {
        return THUMB_LoadStoreHalfword(core, instruction);
    }

    // SP 相對載入/儲存 (1001)
    if ((instruction & 0xF000) == 0x9000) {
        return THUMB_SPRelativeLoadStore(core, instruction);
    }

    // 載入位址 (1010)
    if ((instruction & 0xF000) == 0xA000) {
        return THUMB_LoadAddress(core, instruction);
    }

    // SP 加/減立即數 (1011 000x)
    if ((instruction & 0xFF00) == 0xB000) {
        return THUMB_AddOffsetToSP(core, instruction);
    }

    // PUSH/POP (1011 0x10)
    if ((instruction & 0xF600) == 0xB400) {
        return THUMB_PushPop(core, instruction);
    }

    // 多過載入/儲存 (1100)
    if ((instruction & 0xF000) == 0xC000) {
        return THUMB_MultipleLoadStore(core, instruction);
    }
    
    // 條件分支 (1101xxxx)
    if ((instruction & 0xF000) == 0xD000) {
        uint8_t cond = (instruction >> 8) & 0xF;
        if (cond != 0xF) {  // 不是 SWI
            return THUMB_ConditionalBranch(core, instruction);
        } else {
            return THUMB_SoftwareInterrupt(core, instruction);
        }
    }
    
    // 無條件分支 (11100)
    if ((instruction & 0xF800) == 0xE000) {
        return THUMB_UnconditionalBranch(core, instruction);
    }
    
    // 長分支 (1111x)
    if ((instruction & 0xF000) == 0xF000) {
        return THUMB_LongBranchLink(core, instruction);
    }
    
    #ifdef GBA_DEBUG
    printf("Unimplemented THUMB instruction: 0x%04X at PC=0x%08X\n", 
           instruction, core->cpu.regs.pc);
    #endif
    
    return (InstructionResult) {
        .cycles = 1,
        .branch_taken = false
    };
}
