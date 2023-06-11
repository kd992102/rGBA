#include <string.h>
#include "arm7tdmi.h"
#include "arm_instruction.h"
#include "thumb_instruction.h"
//EQ = 0,NE,CS,CC,MI,PL,VS,VC,HI,LS,GE,LT,LE

void InitCpu(Gba_Cpu *cpu, uint32_t BaseAddr){
    if(cpu->dMode == ARM_MODE){
        cpu->Reg[PC] = BaseAddr + 0x8;
        cpu->fetchcache[2] = MemRead32(cpu, BaseAddr);
        cpu->fetchcache[1] = MemRead32(cpu, BaseAddr + 0x4);
        cpu->fetchcache[0] = MemRead32(cpu, BaseAddr + 0x8);
    }
    else{
        //THUMB_MODE
        cpu->Reg[PC] = BaseAddr + 0x4;
        cpu->fetchcache[2] = MemRead32(cpu, BaseAddr);
        cpu->fetchcache[1] = MemRead32(cpu, BaseAddr + 0x2);
        cpu->fetchcache[0] = MemRead32(cpu, BaseAddr + 0x4);
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

uint32_t MemRead32(Gba_Cpu *cpu, uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    return *((uint32_t *)RelocAddr);
}

uint16_t MemRead16(Gba_Cpu *cpu, uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    return *((uint16_t *)RelocAddr);
}

uint8_t MemRead8(Gba_Cpu *cpu, uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    return *((uint8_t *)RelocAddr);
}

void MemWrite32(Gba_Cpu *cpu, uint32_t addr, uint32_t data){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    *((uint32_t *)RelocAddr) = data;
}

void MemWrite16(Gba_Cpu *cpu, uint32_t addr, uint16_t data){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    *((uint16_t *)RelocAddr) = data;
}

void MemWrite8(Gba_Cpu *cpu, uint32_t addr, uint8_t data){
    uint32_t RelocAddr = MemoryAddrReloc(cpu->GbaMem, addr);
    *((uint8_t *)RelocAddr) = data;
}

void Reset(Gba_Cpu *cpu){
    cpu->Reg_svc[2] = cpu->Reg[PC];
    cpu->SPSR_svc = cpu->CPSR;
    cpu->CPSR = cpu->CPSR & 0xffffff00;//
    cpu->CPSR = cpu->CPSR | 0xd3;//set I、F, clear T
    //
}

void ProcModeChg(Gba_Cpu *cpu){
    uint8_t Mode_bit = cpu->SPSR & 0x1f;
    switch(Mode_bit){
        case 0x10:
            //User
        case 0x11:
            //FIQ
        case 0x12:
            //IRQ
        case 0x13:
            //Supervisor
        case 0x17:
            //ABT
        case 0x1B:
            //UDF
        case 0x1f:
            //System
    }
}

void PreFetch(Gba_Cpu *cpu, uint32_t Addr){
    cpu->Reg[PC] = Addr + 0x8;
    cpu->fetchcache[2] = cpu->fetchcache[1];
    cpu->fetchcache[1] = cpu->fetchcache[0];
    cpu->fetchcache[0] = MemRead32(cpu, cpu->Reg[PC]);
}

void CPSRUpdate(Gba_Cpu *cpu, uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB){
    uint8_t NZCV = 0x0;
    if((result >> 31))NZCV |= 0x8;//N flag

    if(!result)NZCV |= 0x4;//Z flag

    if(LOG){
        if(cpu->carry_out)NZCV |= 0x2;
    }
    else if(A_ADD){
        if(result < parameterA)NZCV |= 0x2;

        //V
        if(!(((parameterA ^ parameterB) >> 31) & 0x1) && (((parameterA ^ result) >> 31) & 0x1))NZCV |= 0x1;
    }
    else if(A_SUB){
        if(parameterA >= parameterB)NZCV |= 0x2;
        //V
        if((((parameterA ^ parameterB) >> 31) & 0x1) && (((parameterA ^ result) >> 31) & 0x1))NZCV |= 0x1;
    }
    cpu->CPSR &= 0xfffffff;
    cpu->CPSR |= (NZCV << 28);
}