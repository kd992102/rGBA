#include <string.h>
#include "arm7tdmi.h"
//EQ = 0,NE,CS,CC,MI,PL,VS,VC,HI,LS,GE,LT,LE

void InitCpu(Gba_Cpu *cpu, uint32_t BaseAddr){
    if(cpu->dMode == ARM_MODE){
        cpu->Reg[PC] = BaseAddr + 0x8;
        cpu->fetchcache[2] = MemRead(cpu, BaseAddr);
        cpu->fetchcache[1] = MemRead(cpu, BaseAddr + 0x4);
        cpu->fetchcache[0] = MemRead(cpu, BaseAddr + 0x8);
    }
    else{
        //THUMB_MODE
        cpu->Reg[PC] = BaseAddr + 0x4;
        cpu->fetchcache[2] = MemRead(cpu, BaseAddr);
        cpu->fetchcache[1] = MemRead(cpu, BaseAddr + 0x2);
        cpu->fetchcache[0] = MemRead(cpu, BaseAddr + 0x4);
    }
    for(int i=0;i<16;i++){
        memset(&cpu->Reg[i],0,4);
    }
    memset(&cpu->CPSR,0,4);
}

uint8_t CheckCond(Gba_Cpu *cpu){
    uint8_t N = ((cpu->CPSR >> 31) & 0x1);
    uint8_t Z = ((cpu->CPSR >> 30) & 0x1);
    uint8_t C = ((cpu->CPSR >> 29) & 0x1);
    uint8_t V = ((cpu->CPSR >> 28) & 0x1);
    switch(cpu->Cond){
        case EQ:
            if(Z == 1)return 1;
            return 0;
        case NE:
            if(Z == 0)return 1;
            return 0;
        case CS:
            if(C == 1)return 1;
            return 0;
        case CC:
            if(C == 0)return 1;
            return 0;
        case MI:
            if(N == 1)return 1;
            return 0;
        case PL:
            if(N == 0)return 1;
            return 0;
        case VS:
            if(V == 1)return 1;
            return 0;
        case VC:
            if(V == 0)return 1;
            return 0;
        case HI:
            if(C == 1 && N == 0)return 1;
            return 0;
        case LS:
            if(C == 0 || Z == 1)return 1;
            return 0;
        case GE:
            if(N == V)return 1;
            return 0;
        case LT:
            if(N != V)return 1;
            return 0;
        case GT:
            if((Z == 0) & (N == V))return 1;
            return 0;
        case LE:
            if((Z == 1) | (N != V))return 1;
            return 0;
        case AL:
            return 1;
        default:
            return 1;
    }
}

uint32_t MemRead(Gba_Cpu *cpu, uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    if(cpu->dMode == ARM_MODE)return *((uint32_t *)RelocAddr);
    else if(cpu->dMode == THUMB_MODE){
        return *((uint16_t *)RelocAddr);
    }
}

void MemWrite(Gba_Cpu *cpu, uint32_t addr, uint32_t data){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    *((uint32_t *)RelocAddr) = data;
}

void PreFetch(Gba_Cpu *cpu, uint32_t Addr){
    cpu->Reg[PC] = Addr + 0x8;
    cpu->fetchcache[2] = cpu->fetchcache[1];
    cpu->fetchcache[1] = cpu->fetchcache[0];
    cpu->fetchcache[0] = MemRead(cpu, cpu->Reg[PC]);
}

uint16_t ThumbModeDecode(Gba_Cpu *cpu, uint16_t inst){
    return 0;
}

uint32_t CpuDecode(Gba_Cpu *cpu, uint32_t inst)
{
    switch(cpu->dMode){
        case ARM_MODE:
            cpu->Cond = (inst >> 28) & 0xf;
            ArmModeDecode(cpu, inst);
            return 0;
        case THUMB_MODE:
            ThumbModeDecode(cpu, inst);
            return 0;
    }
}

void CpuStatus(Gba_Cpu *cpu){
    for(int i=0;i<16;i++){
        printf("R[%02d]:%08x\n", i, cpu->Reg[i]);
    }
    printf("CPSR:%08x\n", cpu->CPSR);
    printf("N:%d,Z:%d,C:%d,V:%d\n", (cpu->CPSR >> 31) & 0x1, (cpu->CPSR >> 30) & 0x1, (cpu->CPSR >> 29) & 0x1, (cpu->CPSR >> 28) & 0x1);
}