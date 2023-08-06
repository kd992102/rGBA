#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
#include "arm_instruction.h"
#include "thumb_instruction.h"
//EQ = 0,NE,CS,CC,MI,PL,VS,VC,HI,LS,GE,LT,LE

extern Gba_Cpu *cpu;
extern GbaMem *Mem;

uint8_t CheckCond(){
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

void CPSRUpdate(uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB){
    uint8_t NZCV = (cpu->CPSR >> 28) & 0xf;
    if((result >> 31))NZCV |= 0x8;//N flag
    else{NZCV &= 0x7;}

    if(!result)NZCV |= 0x4;//Z flag
    else{NZCV &= 0xb;}

    if(Opcode == LOG){
        if(cpu->carry_out)NZCV |= 0x2;
        else{
            NZCV &= 0xd;
        }
    }
    else if(Opcode == A_ADD){
        if(result < parameterA)NZCV |= 0x2;
        else{NZCV &= 0xd;}
        //V
        if(!(((parameterA ^ parameterB) >> 31) & 0x1) && (((parameterA ^ result) >> 31) & 0x1))NZCV |= 0x1;
        else{NZCV &= 0xe;}
    }
    else if(Opcode == A_SUB){
        //printf("A:%08x, B:%08x\n", parameterA, parameterB);
        if(parameterA >= parameterB)NZCV |= 0x2;
        else{NZCV &= 0xd;}
        //V
        if((((parameterA ^ parameterB) >> 31) & 0x1) && (((parameterA ^ result) >> 31) & 0x1))NZCV |= 0x1;
        else{NZCV &= 0xe;}
    }
    cpu->CPSR &= 0xfffffff;
    cpu->CPSR |= (NZCV << 28);
}

void RecoverReg(uint8_t Cmode){
    switch(Cmode){
        case 0x11:
            //FIQ
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
            cpu->Reg_irq[0] = cpu->Reg[SP];
            cpu->Reg_irq[1] = cpu->Reg[LR];
            cpu->SPSR_irq = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        case 0x13:
            //Supervisor
            cpu->Reg_svc[0] = cpu->Reg[SP];
            cpu->Reg_svc[1] = cpu->Reg[LR];
            cpu->SPSR_svc = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        case 0x17:
            //ABT
            cpu->Reg_abt[0] = cpu->Reg[SP];
            cpu->Reg_abt[1] = cpu->Reg[LR];
            cpu->SPSR_abt = cpu->SPSR;

            cpu->Reg[SP] = cpu->Regbk[5];
            cpu->Reg[LR] = cpu->Regbk[6];
            cpu->SPSR = cpu->Regbk[7];
            break;
        case 0x1B:
            //UDF
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

uint8_t ChkCPUMode(){
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

void ArmModeDecode(uint32_t inst){
    cpu->Cond = (inst >> 28) & 0xf;
    if(CheckCond(cpu->CPSR, cpu->Cond)){
        switch(((inst >> 26) & 0x3)){
            case 0:
                if(((inst >> 4) & 0xffffff) == 0x12fff1){
                    ArmBX(inst);
                }
                else{
                    if((((inst >> 4) & 0xf) == 0xb || ((inst >> 4) & 0xf) == 0xf || ((inst >> 4) & 0xf) == 0xd) && ((inst >> 25) & 0x1) == 0 && (((inst >> 24) & 0x1) >= ((inst >> 21) & 0x1))){
                        ArmSDTS(inst);
                    }
                    else if(((inst >> 4) & 0xf) == 9){
                        if(((inst >> 8) & 0xf) == 0 && ((inst >> 20) & 0x3) == 0 && ((inst >> 23) & 0x7) == 2)ArmSWP(inst);
                        else if(((inst >> 23) & 0x7) == 1)ArmMULL(inst);
                        else if(((inst >> 22) & 0xf) == 0)ArmMUL(inst);
                    }
                    else{
                        ArmDataProc(inst);
                    }
                }
                break;
            case 1:
                ArmSDT(inst);
                break;
            case 2:
                if(((inst >> 25) & 0x1)){
                    ArmBranch(inst);
                }
                else{
                    ArmBDT(inst);
                }
                break;
            case 3:
                ArmSWI(inst);
                break;
        }
    }
    else{
    }
}

void ThumbModeDecode(uint16_t inst){
    switch(((inst >> 13) & 0x7)){
        case 0:
            if(((inst >> 11) & 0x3) == 0x3){
                ThumbAS(inst);
            }
            else{
                ThumbMVREG(inst);
            }
            break;
        case 1:
            ThumbIMM(inst);
            break;
        case 2:
            if((inst >> 12) & 0x1){
                if(((inst >> 9) & 0x1))ThumbLSBH(inst);
                else{ThumbLSREG(inst);}
            }
            else{
                if(((inst >> 11) & 0x1)){
                    ThumbPCLOAD(inst);
                }
                else{
                    if(((inst >> 10) & 0x1))ThumbBX(inst);
                    else{
                        ThumbALU(inst);
                    }
                }
            }
            break;
        case 3:
            ThumbLSIMM(inst);
            break;
        case 4:
            if(((inst >> 12) & 0x1))ThumbSPLS(inst);
            else{ThumbLSH(inst);}
            break;
        case 5:
            if(((inst >> 12) & 0x1)){
                //printf("inst : %08x\n", (inst >> 8));
                if(((inst >> 8) & 0xf) == 0x0){
                    //printf("ADDSP\n");
                    ThumbADDSP(inst);
                }
                else{ThumbPPREG(inst);}
            }
            else{
                ThumbLADDR(inst);
            }
            break;
        case 6:
            //printf("case 6\n");
            if(((inst >> 12) & 0x1)){
                //printf("Hey inst:%08x\n", inst);
                if(((inst >> 8) & 0xf) == 0xf)ThumbSWI(inst);
                else{
                    //printf("CondB\n");
                    ThumbCondB(inst);
                }
            }
            else{
                ThumbMULLS(inst);
            }
            break;
        case 7:
            if(((inst >> 12) & 0x1)){
                ThumbLONGBL(inst);
            }
            else{
                ThumbUCOND(inst);
            }
            break;
    }
}

void PreFetch(uint32_t Addr){
    cpu->fetchcache[2] = cpu->fetchcache[1];
    cpu->fetchcache[1] = cpu->fetchcache[0];
    if(cpu->dMode == ARM_MODE)cpu->fetchcache[0] = MemRead32(Addr);
    else{cpu->fetchcache[0] = MemRead16(Addr);}
    cpu->cycle += 1;
}

uint32_t CpuExecute(uint32_t inst)
{
    if(inst == 0x0){
        cpu->cycle += 1;
        return 0;
    }
    switch(cpu->dMode){
        case ARM_MODE:
            cpu->Cond = (inst >> 28) & 0xf;
            ArmModeDecode(inst);
            return 0;
        case THUMB_MODE:
            ThumbModeDecode(inst);
            return 0;
    }
}

void CpuStatus(){
    //system("cls");
    printf("--------register-------\n");
    for(int i=0;i<16;i++){
        printf("R[%02d]:%08x\t", i, cpu->Reg[i]);
        if(i == 3 || i == 7 || i == 11)printf("\n");
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
    printf("Next --> Addr:0x%08x, Instruction:%08x\n", cpu->Reg[PC] - (cpu->InstOffset * 2), cpu->fetchcache[2]);
    printf("fetch:%08x\ndecode:%08x\nexecute:%08x\n", cpu->fetchcache[0], cpu->fetchcache[1], cpu->fetchcache[2]);
    printf("--------End-------\n");
}