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
    cpu->Cmode = SVC_MODE;//SVC Mode
    cpu->CPSR = SVC_MODE;//SVC Mode
    cpu->SPSR_svc = USER_MODE;//USER Mode
    cpu->SPSR = USER_MODE;//USER Mode
    cpu->SPSR_abt = USER_MODE;
    cpu->SPSR_und = USER_MODE;
    cpu->SPSR_irq = USER_MODE;
    cpu->SPSR_fiq = USER_MODE;
    cpu->Reg_svc[0] = 0x3007f00;
    cpu->Reg_svc[1] = 0x8000000;
    cpu->saveMode = ARM_MODE;
    cpu->Halt = 0;
    cpu->CpuMode = SYSTEM_MODE;
    cpu->dMode = ARM_MODE;
    MemWrite16(0x4000088, 0x200);
    //
}