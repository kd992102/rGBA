#include <stdint.h>
#include "arm7tdmi.h"
#include "cpu.h"
#include "arm_instruction.h"
#include "thumb_instruction.h"

void ArmModeDecode(Gba_Cpu *cpu, uint32_t inst){
    //if((inst >> 25) & 0x7 == 0x7)ArmBranch(cpu, inst);
    cpu->Cond = (inst >> 28) & 0xf;
    if(CheckCond(cpu)){
        //printf("Condition True\n");
        switch(((inst >> 26) & 0x3)){
            case 0:
                //printf("arm classification\n");
                if(((inst >> 4) & 0xffffff) == 0x12fff1){
                    //printf("BX start\n");
                    ArmBX(cpu, inst);
                    //PreFetch(cpu, cpu->Ptr);
                    //printf("BX done\n");
                }
                else{
                    if(((inst >> 22) & 0xf) == 0 && ((inst >> 4) & 0xf) == 9){
                        ArmMUL(cpu, inst);
                        //printf("MUL\n");
                    }
                    else if(((inst >> 23) & 0x7) == 0x2 && ((inst >> 20) & 0x3) == 0x0 && ((inst >> 4) & 0xff) == 0x9){
                        ArmSWP(cpu, inst);
                        //printf("SWP\n");
                    }
                    else if(((inst >> 23) & 0x7) == 0x1 && ((inst >> 4) & 0xf) == 0x9){
                        ArmMULL(cpu, inst);
                        //printf("MULL\n");
                    }
                    else if(((inst >> 4) & 0xf) > 0x9 && ((inst >> 4) & 0xf) <= 0xf && ((inst >> 25) & 0x7) == 0){
                        ArmSDTS(cpu, inst);
                        //printf("SDTS\n");
                    }
                    else{ArmDataProc(cpu, inst);/*printf("ALU\n");*/}
                }
                break;
            case 1:
                //else{ArmSDT(cpu, inst);}
                ArmSDT(cpu, inst);
                //printf("SDT\n");
                break;
            case 2:
                if(((inst >> 25) & 0x1)){
                    ArmBranch(cpu, inst);
                    //PreFetch(cpu, cpu->Ptr);
                }
                else{ArmBDT(cpu, inst);/*printf("BDT\n");*/}
                break;
            case 3:
                ArmSWI(cpu, inst);
                //printf("SWI\n");
                break;
        }
    }
    else{
        //printf("Condition False\n");
    }
}

void ThumbModeDecode(Gba_Cpu *cpu, uint16_t inst, uint16_t inst2){
    printf("Thumb Decode:%08x\n", inst);
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
                if((inst >> 11) & 0x1){
                    //printf("PC-relative Load\n");
                    ThumbPCLOAD(cpu, inst);
                }
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
            printf("case 6\n");
            if((inst >> 12) & 0x1){
                printf("Hey inst:%08x\n", inst);
                if(((inst >> 8) & 0xf) == 0xf)ThumbSWI(cpu, inst);
                else{
                    printf("CondB\n");
                    ThumbCondB(cpu, inst);
                }
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

void PreFetch(Gba_Cpu *cpu, uint32_t Addr){
    cpu->fetchcache[2] = cpu->fetchcache[1];
    cpu->fetchcache[1] = cpu->fetchcache[0];
    if(cpu->dMode == ARM_MODE)cpu->fetchcache[0] = MemRead32(cpu, Addr);
    else{cpu->fetchcache[0] = MemRead16(cpu, Addr);}
}

uint32_t CpuExecute(Gba_Cpu *cpu, uint32_t inst)
{
    uint16_t t_inst, t_inst2;
    if(inst == 0x0){return 0;}
    switch(cpu->dMode){
        case ARM_MODE:
            cpu->Cond = (inst >> 28) & 0xf;
            ArmModeDecode(cpu, inst);
            return 0;
        case THUMB_MODE:
            printf("[ change into THUMB mode ]\n");
            printf("inst:%08x, data:%08x, inst1:%08x, data:%08x\n",cpu->fetchcache[0], (uint16_t)(inst & 0xffff), cpu->fetchcache[0], (uint16_t)((inst & 0xffff0000) >> 16));
            t_inst = (uint16_t)(inst & 0xffff);
            t_inst2 = (uint16_t)((inst & 0xffff0000) >> 16);
            ThumbModeDecode(cpu, t_inst, t_inst2);
            return 0;
    }
}

void CpuStatus(Gba_Cpu *cpu){
    //system("cls");
    printf("--------register-------\n");
    for(int i=0;i<16;i++){
        printf("R[%02d]:%08x\t", i, cpu->Reg[i]);
        if(i == 8)printf("\n");
    }

    printf("\n--------status-------\n");
    printf("CPSR:%08x\n", cpu->CPSR);
    switch((cpu->CPSR & 0x1f)){
        case USER:
            printf("Operating mode:User\n");
            break;
        case FIQ:
            printf("Operating mode:FIQ\n");
            break;
        case IRQ:
            printf("Operating mode:IRQ\n");
            break;
        case SVC:
            printf("Operating mode:Supervisor\n");
            break;
        case ABT:
            printf("Operating mode:Abort\n");
            break;
        case UNDEF:
            printf("Operating mode:Undefined\n");
            break;
        case SYSTEM:
            printf("Operating mode:System\n");
            break;
    }
    if(cpu->dMode == ARM_MODE)printf("Decodeing mode:ARM\n");
    else{printf("Decodeing mode:Thumb\n");}
    printf("N:%d,Z:%d,C:%d,V:%d\n", (cpu->CPSR >> 31) & 0x1, (cpu->CPSR >> 30) & 0x1, (cpu->CPSR >> 29) & 0x1, (cpu->CPSR >> 28) & 0x1);

    printf("--------piepeline-------\n");
    printf("Next --> Addr:0x%08x, Instruction:%08x\n", cpu->Ptr - (cpu->InstOffset * 2), cpu->fetchcache[1]);
    printf("fetch:%08x\ndecode:%08x\nexecute:%08x\n", cpu->fetchcache[0], cpu->fetchcache[1], cpu->fetchcache[2]);
    /*printf("--------Mem-------\n");
    for(int y=0x4000000; y < 0x4000300; y+=4){
        if(y % 32 == 0)printf("\n");
        printf("%08x\t", MemRead32(cpu, y));
        //if(y % 16 == 0)printf("\n");
    }*/
    printf("--------End-------\n");
}