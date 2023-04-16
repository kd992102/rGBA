#include "arm_instruction.h"

void ArmDataProc(Gba_Cpu *cpu, uint32_t inst){}
void ArmBranch(Gba_Cpu *cpu, uint32_t inst){
    uint32_t offset;
    if((inst >> 23) & 0x1)offset = (inst << 2) | 0xff000000;
    else{offset = (uint32_t)(inst << 2) & 0x3ffffff;}
    if(CheckCond(cpu)){
        printf("A\n");
        if((inst >> 24) & 0x1){
            cpu->Reg[LR] = cpu->Reg[PC] + 0x4;//next instruction
        }
        cpu->fetchcache[1] = MemRead(cpu, cpu->Reg[PC] + (int32_t)offset);
        cpu->fetchcache[0] = MemRead(cpu, cpu->Reg[PC] + (int32_t)offset + 0x4);
        cpu->fetchcache[2] = 0x0;
        cpu->Ptr = cpu->Reg[PC] + (int32_t)offset - 0x4;
        //printf("Ptr:0x%08x\n", cpu->Ptr);
    }
}
void ArmBX(Gba_Cpu *cpu, uint32_t inst){
    if(CheckCond(cpu)){
        cpu->Reg[PC] = cpu->Reg[(inst & 0xf)];
        if(cpu->Reg[PC] & 0x1)cpu->dMode = THUMB_MODE;
        else{cpu->dMode = ARM_MODE;}
    }
}
void ArmSWP(Gba_Cpu *cpu, uint32_t inst){}
void ArmMUL(Gba_Cpu *cpu, uint32_t inst){}
void ArmMULL(Gba_Cpu *cpu, uint32_t inst){}
void ArmSWI(Gba_Cpu *cpu, uint32_t inst){}
void ArmSDT(Gba_Cpu *cpu, uint32_t inst){}
void ArmSDTS(Gba_Cpu *cpu, uint32_t inst){}
void ArmUDF(Gba_Cpu *cpu, uint32_t inst){}
void ArmBDT(Gba_Cpu *cpu, uint32_t inst){}