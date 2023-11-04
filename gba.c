#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "arm7tdmi.h"
#include "gba.h"

extern Gba_Cpu *cpu;
IntMask IntMasks;

void InitCpu(uint32_t BaseAddr){
    for(int i=0;i<16;i++){
        memset(&cpu->Reg[i],0,4);
    }
    memset(&cpu->CPSR,0,4);
    //change to Supervisor mode
    Reset(cpu);
    
    cpu->Reg[PC] = BaseAddr + 0x8;
    cpu->fetchcache[2] = MemRead32(BaseAddr);
    cpu->fetchcache[1] = MemRead32(BaseAddr + 0x4);
    cpu->fetchcache[0] = MemRead32(BaseAddr + 0x8);
}

void Reset(){
    cpu->Reg_svc[2] = cpu->Reg[PC];//R14 of supervisor mode
    cpu->SPSR_svc = cpu->CPSR;//backup CPSR
    cpu->CPSR = cpu->CPSR & 0xffffff00;//clear the I、F、T and mode bit
    cpu->CPSR = cpu->CPSR | 0x1F;//set I、F, clear T
    cpu->Reg[SP] = 0x3007F00;
    MemWrite16(0x4000088, 0x200);
    //
}

void IRQhandler(){
    for(int i=0;i<13;i++){
        if()
    }
}