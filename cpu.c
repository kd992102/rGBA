#include <stdint.h>
#include "arm7tdmi.h"
#include "cpu.h"
#include "arm_instruction.h"
#include "thumb_instruction.h"

void RecoverReg(Gba_Cpu *cpu, uint8_t Cmode){
    switch(Cmode){
        case 0x11:
            //FIQ
            printf("FIQ\n");
            cpu->Reg_fiq[0] = cpu->Reg[R8];
            cpu->Reg_fiq[1] = cpu->Reg[R9];
            cpu->Reg_fiq[2] = cpu->Reg[R10];
            cpu->Reg_fiq[3] = cpu->Reg[R11];
            cpu->Reg_fiq[4] = cpu->Reg[R12];
            cpu->Reg_fiq[5] = cpu->Reg[SP];
            cpu->Reg_fiq[6] = cpu->Reg[LR];
            cpu->SPSR_fiq = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            cpu->Reg[R8] = cpu->Regbk[0];
            cpu->Reg[R9] = cpu->Regbk[1];
            cpu->Reg[R10] = cpu->Regbk[2];
            cpu->Reg[R11] = cpu->Regbk[3];
            cpu->Reg[R12] = cpu->Regbk[4];
            break;
        case 0x12:
            //IRQ
            printf("IRQ\n");
            cpu->Reg_irq[0] = cpu->Reg[SP];
            cpu->Reg_irq[1] = cpu->Reg[LR];
            cpu->SPSR_irq = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        case 0x13:
            //Supervisor
            printf("SVC\n");
            cpu->Reg_svc[0] = cpu->Reg[SP];
            cpu->Reg_svc[1] = cpu->Reg[LR];
            cpu->SPSR_svc = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        case 0x17:
            //ABT
            printf("ABT\n");
            cpu->Reg_abt[0] = cpu->Reg[SP];
            cpu->Reg_abt[1] = cpu->Reg[LR];
            cpu->SPSR_abt = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        case 0x1B:
            //UDF
            printf("UDF\n");
            cpu->Reg_und[0] = cpu->Reg[SP];
            cpu->Reg_und[1] = cpu->Reg[LR];
            cpu->SPSR_und = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        default:
            for(int i=0;i<8;i++){
                cpu->Regbk[i] = 0x0;
            }
            break;
    }
}

uint8_t ChkCPUMode(Gba_Cpu *cpu){
    switch((cpu->CPSR & 0x1F)){
        case 0x10:
            //User
            cpu->CpuMode = USER;
            return USER;
        case 0x11:
            //FIQ
            cpu->CpuMode = FIQ;
            cpu->Regbk[5] = cpu->Reg[SP];
            cpu->Regbk[6] = cpu->Reg[LR];
            cpu->Regbk[7] = cpu->SPSR;
            cpu->Regbk[0] = cpu->Reg[R8];
            cpu->Regbk[1] = cpu->Reg[R9];
            cpu->Regbk[2] = cpu->Reg[R10];
            cpu->Regbk[3] = cpu->Reg[R11];
            cpu->Regbk[4] = cpu->Reg[R12];
            cpu->Reg[R8] = cpu->Reg_fiq[0];
            cpu->Reg[R9] = cpu->Reg_fiq[1];
            cpu->Reg[R10] = cpu->Reg_fiq[2];
            cpu->Reg[R11] = cpu->Reg_fiq[3];
            cpu->Reg[R12] = cpu->Reg_fiq[4];
            cpu->Reg[SP] = cpu->Reg_fiq[5];
            cpu->Reg[LR] = cpu->Reg_fiq[6];
            cpu->SPSR = cpu->SPSR_fiq;
            return FIQ;
        case 0x12:
            //IRQ
            cpu->CpuMode = IRQ;
            cpu->Regbk[5] = cpu->Reg[SP];
            cpu->Regbk[6] = cpu->Reg[LR];
            cpu->Regbk[7] = cpu->SPSR;
            cpu->Reg[SP] = cpu->Reg_irq[0];
            cpu->Reg[LR] = cpu->Reg_irq[1];
            cpu->SPSR = cpu->SPSR_irq;
            return IRQ;
        case 0x13:
            //Supervisor
            cpu->CpuMode = SVC;
            cpu->Regbk[5] = cpu->Reg[SP];
            cpu->Regbk[6] = cpu->Reg[LR];
            cpu->Regbk[7] = cpu->SPSR;
            cpu->Reg[SP] = cpu->Reg_svc[0];
            cpu->Reg[LR] = cpu->Reg_svc[1];
            cpu->SPSR = cpu->SPSR_svc;
            return SVC;
        case 0x17:
            //ABT
            cpu->CpuMode = ABT;
            cpu->Regbk[5] = cpu->Reg[SP];
            cpu->Regbk[6] = cpu->Reg[LR];
            cpu->Regbk[7] = cpu->SPSR;
            cpu->Reg[SP] = cpu->Reg_abt[0];
            cpu->Reg[LR] = cpu->Reg_abt[1];
            cpu->SPSR = cpu->SPSR_abt;
            return ABT;
        case 0x1B:
            //UDF
            cpu->CpuMode = UNDEF;
            cpu->Regbk[5] = cpu->Reg[SP];
            cpu->Regbk[6] = cpu->Reg[LR];
            cpu->Regbk[7] = cpu->SPSR;
            cpu->Reg[SP] = cpu->Reg_und[0];
            cpu->Reg[LR] = cpu->Reg_und[1];
            cpu->SPSR = cpu->SPSR_und;
            return UNDEF;
        case 0x1F:
            //System
            cpu->CpuMode = SYSTEM;
            return SYSTEM;
    }
}

void ArmModeDecode(Gba_Cpu *cpu, uint32_t inst){
    //if((inst >> 25) & 0x7 == 0x7)ArmBranch(cpu, inst);
    cpu->Cond = (inst >> 28) & 0xf;
    if(CheckCond(cpu)){
        //printf("Condition True\n");
        switch(((inst >> 26) & 0x3)){
            case 0:
                //printf("arm classification\n");
                if(((inst >> 4) & 0xffffff) == 0x12fff1){
                    ArmBX(cpu, inst);
                    cpu->cycle += 3;
                }
                else{
                    if(((inst >> 22) & 0xf) == 0 && ((inst >> 4) & 0xf) == 9){
                        ArmMUL(cpu, inst);
                        cpu->cycle += 1;
                    }
                    else if(((inst >> 23) & 0x7) == 0x2 && ((inst >> 20) & 0x3) == 0x0 && ((inst >> 4) & 0xff) == 0x9){
                        ArmSWP(cpu, inst);
                        cpu->cycle += 1;
                    }
                    else if(((inst >> 23) & 0x7) == 0x1 && ((inst >> 4) & 0xf) == 0x9){
                        ArmMULL(cpu, inst);
                        cpu->cycle += 1;
                    }
                    else if(((inst >> 4) & 0xf) > 0x9 && ((inst >> 4) & 0xf) <= 0xf && ((inst >> 25) & 0x7) == 0){
                        ArmSDTS(cpu, inst);
                    }
                    else{
                        ArmDataProc(cpu, inst);
                        cpu->cycle += 1;
                        //printf("cpu->Cycle:%d, cpu->CPSR:%08x\n", cpu->cycle, cpu->CPSR);
                    }
                }
                break;
            case 1:
                ArmSDT(cpu, inst);
                break;
            case 2:
                if(((inst >> 25) & 0x1)){
                    ArmBranch(cpu, inst);
                }
                else{
                    ArmBDT(cpu, inst);
                    cpu->cycle += 1;
                }
                break;
            case 3:
                ArmSWI(cpu, inst);
                cpu->cycle += 1;
                break;
        }
    }
    else{
        cpu->cycle += 1;
    }
}

void ThumbModeDecode(Gba_Cpu *cpu, uint16_t inst){
    //printf("Thumb Decode:%08x\n", inst);
    switch((inst >> 13) & 0x7){
        case 0:
            if((inst >> 11) & 0x3 == 0x3)ThumbAS(cpu, inst);
            else{ThumbMVREG(cpu, inst);}
            cpu->cycle += 1;
            break;
        case 1:
            ThumbIMM(cpu, inst);
            cpu->cycle += 1;
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
                    cpu->cycle += 3;
                }
                else{
                    if((inst >> 10) & 0x1)ThumbBX(cpu, inst);
                    else{
                        ThumbALU(cpu, inst);
                        cpu->cycle += 1;
                    }
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
                //printf("inst : %08x\n", (inst >> 8));
                if(((inst >> 8) & 0xf) == 0x0){
                    //printf("ADDSP\n");
                    ThumbADDSP(cpu, inst);
                }
                else{ThumbPPREG(cpu, inst);}
            }
            else{
                ThumbLADDR(cpu, inst);
            }
            break;
        case 6:
            //printf("case 6\n");
            if((inst >> 12) & 0x1){
                //printf("Hey inst:%08x\n", inst);
                if(((inst >> 8) & 0xf) == 0xf)ThumbSWI(cpu, inst);
                else{
                    //printf("CondB\n");
                    ThumbCondB(cpu, inst);
                }
            }
            else{
                ThumbMULLS(cpu, inst);
            }
            break;
        case 7:
            if((inst >> 12) & 0x1){
                printf("Long Branch\n");
                ThumbLONGBL(cpu, inst);
            }
            else{
                ThumbUCOND(cpu, inst);
            }
            break;
    }
}

void PreFetch(Gba_Cpu *cpu, uint32_t Addr){
    cpu->fetchcache[2] = cpu->fetchcache[1];
    cpu->fetchcache[1] = cpu->fetchcache[0];
    //printf("Stuck here!\n");
    if(cpu->dMode == ARM_MODE)cpu->fetchcache[0] = MemRead32(cpu, Addr);
    else{cpu->fetchcache[0] = MemRead16(cpu, Addr);}
    //printf("End here!\n");
}

uint32_t CpuExecute(Gba_Cpu *cpu, uint32_t inst)
{
    uint16_t t_inst, t_inst2;
    if(inst == 0x0){
        cpu->cycle += 1;
        return 0;
    }
    switch(cpu->dMode){
        case ARM_MODE:
            cpu->Cond = (inst >> 28) & 0xf;
            ArmModeDecode(cpu, inst);
            return 0;
        case THUMB_MODE:
            //printf("[ change into THUMB mode ]\n");
            //printf("inst:%08x, data:%08x, inst1:%08x, data:%08x\n",cpu->fetchcache[0], (uint16_t)(inst & 0xffff), cpu->fetchcache[0], (uint16_t)((inst & 0xffff0000) >> 16));
            ThumbModeDecode(cpu, inst);
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
    printf("--------End-------\n");
}