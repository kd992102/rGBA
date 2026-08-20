#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "arm7tdmi.h"
#include "gba.h"

void GBA_Init(GBA_Context *ctx){
    Memory_Init(&ctx->mem);

    for(int i=0;i<31;i++){
        ctx->cpu.phys_regs[i] = 0;
    }
    for(int i=0;i<32;i++){
        ctx->cpu.SPSRs[i] = 0;
    }
    
    ctx->cpu.cycle = 0;
    ctx->cpu.cycle_sum = 0;
    ctx->cpu.CPSR = 0x1F; // System mode or SVC (0x13)? Previous was 0x13
    
    // Change to Supervisor mode (0x13)
    ctx->cpu.CPSR = 0x13;
    
    // SVC banked registers initialization
    ctx->cpu.SPSRs[SVC] = 0x10;
    ctx->cpu.phys_regs[REG_MAP[SVC][13]] = 0x3007f00; // SP_svc
    ctx->cpu.phys_regs[REG_MAP[SVC][14]] = 0x8000000; // LR_svc
    
    ctx->cpu.Halt = 0;
    
    // Write 0x200 to 0x4000088
    MemWrite16(ctx, 0x4000088, 0x200);
    
    uint32_t BaseAddr = 0x08000000;
    set_Reg(&ctx->cpu, PC, BaseAddr + 0x8);
    
    ctx->cpu.fetchcache[2] = MemRead32(ctx, BaseAddr);
    ctx->cpu.CurrentInstAddr[2] = BaseAddr;
    ctx->cpu.fetchcache[1] = MemRead32(ctx, BaseAddr + 0x4);
    ctx->cpu.CurrentInstAddr[1] = BaseAddr + 0x4;
    ctx->cpu.fetchcache[0] = MemRead32(ctx, BaseAddr + 0x8);
    ctx->cpu.CurrentInstAddr[0] = BaseAddr + 0x8;
}

void GBA_Step(GBA_Context *ctx) {
    if (!ctx->cpu.Halt) {
        // Implement pipeline advance and execution
        // Example:
        // uint32_t inst = ctx->cpu.fetchcache[2];
        // CpuExecute(ctx, inst);
        // PreFetch(ctx);
    }
}
