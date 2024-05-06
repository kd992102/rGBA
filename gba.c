#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "arm7tdmi.h"
#include "gba.h"

extern Gba_Cpu *cpu;

void InitCpu(uint32_t BaseAddr){
    for(int i=0;i<16;i++){
        memset(&cpu->Reg[i],0,4);
    }
    cpu->cycle = 0;
    cpu->cycle_sum = 0;
    cpu->Cmode = 0x13;//SVC mode
    cpu->CPSR = 0x13;
    cpu->SPSR_svc = 0x10;
    cpu->Reg_svc[0] = 0x3007f00;
    cpu->Reg_svc[1] = 0x8000000;
    cpu->saveMode = ARM_MODE;
    cpu->Halt = 0;
    memset(&cpu->CPSR,0,4);
    //change to Supervisor mode
    Reset(cpu);
    
    cpu->Reg[PC] = BaseAddr + 0x8;
    cpu->fetchcache[2] = MemRead32(BaseAddr);
    cpu->CurrentInstAddr[2] = BaseAddr;
    cpu->fetchcache[1] = MemRead32(BaseAddr + 0x4);
    cpu->CurrentInstAddr[1] = BaseAddr + 0x4;
    cpu->fetchcache[0] = MemRead32(BaseAddr + 0x8);
    cpu->CurrentInstAddr[0] = BaseAddr + 0x8;
}

void Reset(){
    cpu->cycle = 0;
    cpu->frame_cycle = 0;
    cpu->cycle_sum = 0;
    cpu->Cmode = 0x13;//SVC mode
    cpu->CPSR = 0x13;
    cpu->SPSR_svc = 0x10;
    cpu->SPSR = 0x10;
    cpu->SPSR_abt = 0x10;
    cpu->SPSR_und = 0x10;
    cpu->SPSR_irq = 0x10;
    cpu->SPSR_fiq = 0x10;
    cpu->Reg_svc[0] = 0x3007f00;
    cpu->Reg_svc[1] = 0x8000000;
    cpu->saveMode = ARM_MODE;
    cpu->Halt = 0;
    MemWrite16(0x4000088, 0x200);
    //
}