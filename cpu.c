#include <stdint.h>
#include "arm7tdmi.h"
#include "cpu.h"
#include "arm_instruction.h"
#include "thumb_instruction.h"

void ArmModeDecode(Gba_Cpu *cpu, uint32_t inst){
    //if((inst >> 25) & 0x7 == 0x7)ArmBranch(cpu, inst);
    cpu->Cond = (inst >> 28) & 0xf;
    if(CheckCond(cpu)){
        switch(((inst >> 26) & 0x3)){
            case 0:
                if(((inst >> 4) & 0xffffff) == 0x12fff1)ArmBX(cpu, inst);
                else{
                    if(((inst >> 22) & 0x7) == 0 && ((inst >> 4) & 0xf) == 9)ArmMUL(cpu, inst);
                    else if(((inst >> 20) & 0x1b == 0x10 && ((inst >> 4) & 0xff) == 0x9))ArmSWP(cpu, inst);
                    else if(((inst >> 7) & 0x1f) == 0x1 && ((inst >> 4) & 0x1) == 1)ArmSDTS(cpu, inst);
                    else if(((inst >> 23) & 0x7) == 0x1 && ((inst >> 4) & 0xf) == 0x9)ArmMULL(cpu, inst);
                    else{ArmDataProc(cpu, inst);}
                }
                break;
            case 1:
                //if(((inst >> 25) & 0x1) && ((inst >> 4) & 0x1))Undefined(cpu);
                //else{ArmSDT(cpu, inst);}
                ArmSDT(cpu, inst);
                break;
            case 2:
                if(((inst >> 25) & 0x1))ArmBranch(cpu, inst);
                else{ArmBDT(cpu, inst);}
                break;
            case 3:
                ArmSWI(cpu, inst);
                break;
        }
    }
}

void ThumbModeDecode(Gba_Cpu *cpu, uint16_t inst, uint16_t inst2){
    switch((inst >> 13) & 0x7){
        case 0:
            if((inst >> 11) & 0x3 == 0x3)ThumbAS(cpu, inst);
            else{ThumbMVREG(cpu, inst);}
            break;
        case 1:
            ThumbIMM(cpu, inst);
            break;
        case 2:
            if((inst >> 12) & 0x1){
                if((inst >> 9) & 0x1)ThumbLSBH(cpu, inst);
                else{ThumbLSREG(cpu, inst);}
            }
            else{
                if((inst >> 11) & 0x1)ThumbPCLOAD(cpu, inst);
                else{
                    if((inst >> 10) & 0x1)ThumbBX(cpu, inst);
                    else{ThumbALU(cpu, inst);}
                }
            }
            break;
        case 3:
            ThumbLSIMM(cpu, inst);
            break;
        case 4:
            if((inst >> 12) & 0x1)ThumbLSH(cpu, inst);
            else{ThumbSPLS(cpu, inst);}
            break;
        case 5:
            if((inst >> 13) & 0x1){
                if((inst >> 8) & 0xf == 0x0)ThumbADDSP(cpu, inst);
                else{ThumbPPREG(cpu, inst);}
            }
            else{
                ThumbLADDR(cpu, inst);
            }
            break;
        case 6:
            if((inst >> 13) & 0x1){
                if((inst >> 8) & 0xf == 0xf)ThumbSWI(cpu, inst);
                else{ThumbCondB(cpu, inst);}
            }
            else{
                ThumbMULLS(cpu, inst);
            }
            break;
        case 7:
            if((inst >> 12) & 0x1)ThumbUCOND(cpu, inst);
            else{ThumbLONGBL(cpu, inst, inst2);}
            break;
    }
}

uint32_t CpuDecode(Gba_Cpu *cpu, uint32_t inst)
{
    uint16_t t_inst, t_inst2;
    switch(cpu->dMode){
        case ARM_MODE:
            cpu->Cond = (inst >> 28) & 0xf;
            ArmModeDecode(cpu, inst);
            return 0;
        case THUMB_MODE:
            t_inst = *((uint16_t *)inst);
            t_inst2 = *(((uint16_t *)inst) + 1);
            ThumbModeDecode(cpu, t_inst, t_inst2);
            return 0;
    }
}

void CpuStatus(Gba_Cpu *cpu){
    printf("--------register-------\n");
    for(int i=0;i<16;i++){
        printf("R[%02d]:%08x\n", i, cpu->Reg[i]);
    }
    printf("--------piepeline-------\n");
    printf("Current Instruction:%08x\n", cpu->Ptr);
    printf("fetch:%08x\ndecode:%08x\nexecute:%08x\n", cpu->fetchcache[2], cpu->fetchcache[1], cpu->fetchcache[0]);
    printf("--------status-------\n");
    printf("CPSR:%08x\n", cpu->CPSR);
    printf("N:%d,Z:%d,C:%d,V:%d\n", (cpu->CPSR >> 31) & 0x1, (cpu->CPSR >> 30) & 0x1, (cpu->CPSR >> 29) & 0x1, (cpu->CPSR >> 28) & 0x1);
}