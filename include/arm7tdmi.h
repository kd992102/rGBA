#ifndef ARM7TDMI_H
#define ARM7TDMI_H

typedef struct GBA_Context GBA_Context;

#include <stdint.h>
#include <stdbool.h>
#define A_ADD 1
#define A_SUB 0
#define LOG 2
#define MOVS 3

enum DECODE_MODE{
    ARM_MODE = 0,THUMB_MODE
};

enum COND{
    EQ = 0,NE,CS,CC,MI,PL,VS,VC,HI,LS,GE,LT,GT,LE,AL
};

enum ARM_MODE_REG{
    R0 = 0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,SP,LR,PC,CPSR
};

enum CPU_MODE{
    USER = 16,FIQ = 17,IRQ = 18,SVC = 19,ABT = 23,UNDEF = 27, SYSTEM = 31
};

extern const uint8_t REG_MAP[32][16];

typedef struct arm7tdmi{
    // === 64-bit Variables ===
    uint32_t frame_cycle;
    uint64_t cycle;
    uint64_t cycle_sum;
    uint32_t CurrentInstAddr[3];
    uint32_t CurrentInst;
    uint32_t fetchcache[3];

    // General & Banked Registers
    uint32_t phys_regs[31];           // System & User (R0-R15)

    // Status Registers & Backups
    uint32_t CPSR;
    uint32_t SPSRs[32];

    // Pipeline & Execution State
    uint32_t prefetch_opcode;

    // === 8-bit Variables ===
    uint8_t Halt;
    uint8_t DebugFunc;
} Gba_Cpu;

static inline uint32_t get_SPSR(Gba_Cpu* cpu) {
    return cpu->SPSRs[cpu->CPSR & 0x1F]; 
}
static inline void set_SPSR(Gba_Cpu* cpu, uint32_t value) {
    cpu->SPSRs[cpu->CPSR & 0x1F] = value;
}

static inline uint32_t get_Reg(Gba_Cpu* cpu, uint8_t reg_index) {
    return cpu->phys_regs[REG_MAP[cpu->CPSR & 0x1F][reg_index]];
}
static inline void set_Reg(Gba_Cpu* cpu, uint8_t reg_index, uint32_t value) {
    cpu->phys_regs[REG_MAP[cpu->CPSR & 0x1F][reg_index]] = value;
}

static inline uint8_t CheckCond(uint32_t cpsr, uint8_t cond) {
    uint32_t N = cpsr & (1 << 31);
    uint32_t Z = cpsr & (1 << 30);
    uint32_t C = cpsr & (1 << 29);
    uint32_t V = cpsr & (1 << 28);
    switch(cond) {
        case EQ: return Z != 0;
        case NE: return Z == 0;
        case CS: return C != 0;
        case CC: return C == 0;
        case MI: return N != 0;
        case PL: return N == 0;
        case VS: return V != 0;
        case VC: return V == 0;
        case HI: return C != 0 && Z == 0;
        case LS: return C == 0 || Z != 0;
        case GE: return (N != 0) == (V != 0);
        case LT: return (N != 0) != (V != 0);
        case GT: return Z == 0 && ((N != 0) == (V != 0));
        case LE: return Z != 0 || ((N != 0) != (V != 0));
        case AL: return 1;
        default: return 1;
    }
}
void CPSRUpdate(GBA_Context *ctx, uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB, uint8_t shifter_carry_out);

void pipeline_flush(GBA_Context *ctx);
void ErrorHandler(GBA_Context *ctx);
void RecoverReg(GBA_Context *ctx, uint8_t Cmode);
uint8_t ChkCPUMode(GBA_Context *ctx);
uint32_t CpuExecute(GBA_Context *ctx, uint32_t inst);
void CpuStatus(GBA_Context *ctx);
void PreFetch(GBA_Context *ctx);
void ThumbModeDecode(GBA_Context *ctx, uint16_t inst);
void ArmModeDecode(GBA_Context *ctx, uint32_t inst);
void IRQ_checker(GBA_Context *ctx, uint32_t CPSR);
void IRQ_handler(GBA_Context *ctx);

#endif