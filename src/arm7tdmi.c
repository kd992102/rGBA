#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
#include "gba.h"

//EQ = 0,NE,CS,CC,MI,PL,VS,VC,HI,LS,GE,LT,LE

const uint8_t REG_MAP[32][16] = {
    // System (31) & User (16)
    [16] = {0,1,2,3,4,5,6,7, 8,9,10,11,12, 13,14, 15},
    [31] = {0,1,2,3,4,5,6,7, 8,9,10,11,12, 13,14, 15},
    // FIQ (17)
    [17] = {0,1,2,3,4,5,6,7, 16,17,18,19,20, 21,22, 15},
    // IRQ (18)
    [18] = {0,1,2,3,4,5,6,7, 8,9,10,11,12, 23,24, 15},
    // SVC (19)
    [19] = {0,1,2,3,4,5,6,7, 8,9,10,11,12, 25,26, 15},
    // ABT (23)
    [23] = {0,1,2,3,4,5,6,7, 8,9,10,11,12, 27,28, 15},
    // UNDEF (27)
    [27] = {0,1,2,3,4,5,6,7, 8,9,10,11,12, 29,30, 15},
};

void pipeline_flush(GBA_Context *ctx){
    ctx->cpu.prefetch_opcode = 0;
}

// CheckCond is moved to arm7tdmi.h as an inline function for performance.

void CPSRUpdate(GBA_Context *ctx, uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB, uint8_t shifter_carry_out){
    uint8_t NZCV = (ctx->cpu.CPSR >> 28) & 0xf;
    if((result >> 31))NZCV |= 0x8;//N flag
    else{NZCV &= 0x7;}

    if(!result)NZCV |= 0x4;//Z flag
    else{NZCV &= 0xb;}

    if(Opcode == LOG){
        if(shifter_carry_out == 1)NZCV |= 0x2;
        else if(shifter_carry_out == 2) { /* Do not modify carry flag */ }
        else{
            NZCV &= 0xd;
        }
    }
    else if(Opcode == MOVS){
        /* Do not modify carry flag */
    }
    else if(Opcode == A_ADD){
        if(result < parameterA)NZCV |= 0x2;
        else{NZCV &= 0xd;}
        //V
        if(!(((parameterA ^ parameterB) >> 31) & 0x1) && (((parameterA ^ result) >> 31) & 0x1))NZCV |= 0x1;
        else{NZCV &= 0xe;}
    }
    else if(Opcode == A_SUB){
        if(parameterA >= parameterB)NZCV |= 0x2;
        else{NZCV &= 0xd;}
        //V
        if((((parameterA ^ parameterB) >> 31) & 0x1) && (((parameterA ^ result) >> 31) & 0x1))NZCV |= 0x1;
        else{NZCV &= 0xe;}
    }
    ctx->cpu.CPSR &= 0xfffffff;
    ctx->cpu.CPSR |= (NZCV << 28);
    //ctx->cpu.carry_out = 0;
}

uint8_t ChkCPUMode(GBA_Context *ctx){
    return ctx->cpu.CPSR & 0x1F;
}

void PreFetch(GBA_Context *ctx){
    uint32_t pc = get_Reg(&ctx->cpu, PC);
    if(ctx->cpu.CPSR & 0x20){ // THUMB_MODE
        ctx->cpu.prefetch_opcode = MemRead16(ctx, pc);
        set_Reg(&ctx->cpu, PC, pc + 2);
    } else { // ARM_MODE
        ctx->cpu.prefetch_opcode = MemRead32(ctx, pc);
        set_Reg(&ctx->cpu, PC, pc + 4);
    }
}

void IRQ_checker(GBA_Context *ctx, uint32_t CPSR){
    if(!((CPSR >> 7) & 0x1)){
        if(MemRead8(ctx, 0x4000208)){//IME
            if((MemRead8(ctx, 0x4000200) & MemRead8(ctx, 0x4000202))){//IF、IE
                IRQ_handler(ctx);
            }
        }
    }
}

void IRQ_handler(GBA_Context *ctx){
    // Save PC to IRQ's LR (R14)
    uint32_t pc_val = get_Reg(&ctx->cpu, PC);
    
    // Switch to IRQ Mode and ARM Mode, Disable IRQ
    uint32_t old_cpsr = ctx->cpu.CPSR;
    ctx->cpu.CPSR = (ctx->cpu.CPSR & ~0x3F) | 0x92; // 0x92 = IRQ mode (0x12) + I bit (0x80) + ARM mode (T=0)
    
    // Now we are in IRQ mode, set LR and SPSR
    set_Reg(&ctx->cpu, LR, pc_val);
    set_SPSR(&ctx->cpu, old_cpsr);
    
    // Jump to vector 0x18
    set_Reg(&ctx->cpu, PC, 0x18);
    
    // Note: Assuming fetchcache is being replaced by prefetch_opcode
    // we keep the memory reads here if the array is still defined in other files, 
    // but typically you would just flush the pipeline here.
    // ctx->cpu.fetchcache[2] = MemRead32(ctx, get_Reg(&ctx->cpu, PC));
    // ctx->cpu.fetchcache[1] = MemRead32(ctx, get_Reg(&ctx->cpu, PC) + 0x4);
    // ctx->cpu.fetchcache[0] = MemRead32(ctx, get_Reg(&ctx->cpu, PC) + 0x8);
    set_Reg(&ctx->cpu, PC, get_Reg(&ctx->cpu, PC) + 0x8);
    ctx->cpu.Halt = 0;
}