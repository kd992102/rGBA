#include <stdint.h>
#include "arm7tdmi.h"
#include "arm_instruction.h"

uint32_t BarrelShifter(Gba_Cpu *cpu, uint32_t Opr, uint8_t Imm, uint32_t result){
    uint8_t shift_type;
    uint8_t shift;
    if(Imm){
        shift = (((Opr >> 8) & 0xf) << 1);
        cpu->carry_out = ((Opr & 0xff) >> (shift - 1)) & 0x1;
        if(shift != 0)result = ((Opr & 0xff) << (32 - shift)) | ((Opr & 0xff) >> shift);
        else{result = Opr & 0xff;}
    }
    else{
        shift_type = (Opr >> 5) & 0x3;
        if((Opr >> 4) & 0x1){
            shift = cpu->Reg[(Opr >> 8) & 0xf] & 0xf;
            if(!shift)cpu->carry_out = (cpu->CPSR >> 29) & 0x1;
            cpu->cycle += 1;//1I
        }
        else{
            shift = (Opr >> 7) & 0x1f;
        }
        switch(shift_type){
            case LSL:
                result = cpu->Reg[(Opr & 0xf)] << shift;
                if(shift != 0)cpu->carry_out = (cpu->Reg[(Opr & 0xf)] >> (32 - shift)) & 0x1;
                break;
            case LSR:
                result = cpu->Reg[(Opr & 0xf)] >> shift;
                if(shift != 0)cpu->carry_out = (cpu->Reg[(Opr & 0xf)] >> (shift - 1)) & 0x1;
                break;
            case ASR:
                result = (uint32_t)((int32_t)(cpu->Reg[(Opr & 0xf)] >> shift));
                if(shift != 0)cpu->carry_out = (cpu->Reg[(Opr & 0xf)] >> (shift - 1)) & 0x1;
                break;
            case ROR:
                if(shift == 0){
                    cpu->carry_out = cpu->Reg[(Opr & 0xf)] & 0x1;
                    result = (((cpu->CPSR >> 29) & 0x1) << 31) | (cpu->Reg[(Opr & 0xf)] >> 1);
                }
                else if(shift != 0){
                    result = (cpu->Reg[(Opr & 0xf)] >> shift) | (cpu->Reg[(Opr & 0xf)] >> (32 - shift));
                    cpu->carry_out = (cpu->Reg[(Opr & 0xf)] >> (shift - 1)) & 0x1;
                }
                break;
            //extended?
        }
    }
    //if(S_bit)cpu->CPSR = cpu->CPSR & (cpu->carry_out << 29);//Update C flag
    return result;
}

void ArmPSRT(Gba_Cpu *cpu, uint32_t inst){
    uint8_t psr = (inst >> 22) & 0x1;
    uint8_t bit = (inst >> 21) & 0x1;
    uint8_t Imm = (inst >> 25) & 0x1;
    uint8_t b_only = (inst >> 16) & 0x1;
    uint8_t Rd = (inst >> 12) & 0xf;
    uint8_t Rm = (inst & 0xf);
    uint32_t Opr = (inst & 0xfff);
    uint32_t res;
    //printf("Cycle:%d, PSRT\n", cpu->cycle);
    if(bit){
        //MSR
        if(b_only){
            if(psr)cpu->SPSR = cpu->Reg[Rm];
            else{cpu->CPSR = cpu->Reg[Rm];}
        }
        else{
            res = BarrelShifter(cpu, Opr, Imm, res);
            if(psr)cpu->SPSR = (cpu->SPSR & 0xfffffff) | (res & 0xf0000000);
            else{cpu->CPSR = (cpu->CPSR & 0xfffffff) | (res & 0xf0000000);}
        }
    }
    else{
        //MRS
        if(psr){cpu->Reg[Rd] = cpu->SPSR;}
        else{
            cpu->Reg[Rd] = cpu->CPSR;
        }
    }
}

void ArmDataProc(Gba_Cpu *cpu, uint32_t inst){
    uint8_t Opcode = (inst >> 21) & 0xf;
    uint8_t Rn = (inst >> 16) & 0xf;
    uint8_t Rd = (inst >> 12) & 0xf;
    uint32_t Opr2 = (inst) & 0xfff;
    uint32_t exOpr;
    uint8_t S_bit = (inst >> 20) & 0x1;
    exOpr = BarrelShifter(cpu, Opr2, (inst >> 25) & 0x1, exOpr);
    switch(Opcode){
        case AND://logical
            cpu->Reg[Rd] = cpu->Reg[Rn] & exOpr;
            if(S_bit)CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case EOR://logical
            cpu->Reg[Rd] = cpu->Reg[Rn] ^ exOpr;
            if(S_bit)CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case SUB://arith
            cpu->Reg[Rd] = cpu->Reg[Rn] - exOpr;
            if(S_bit)CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case RSB://arith
            cpu->Reg[Rd] = exOpr - cpu->Reg[Rn];
            if(S_bit)CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], exOpr, cpu->Reg[Rn]);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case ADD://arith
            cpu->Reg[Rd] = cpu->Reg[Rn] + exOpr;
            if(S_bit)CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case ADC://arith
            cpu->Reg[Rd] = cpu->Reg[Rn] + exOpr + ((cpu->CPSR >> 29) & 0x1);
            if(S_bit)CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rn] + ((cpu->CPSR >> 29) & 0x1), exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case SBC://arith
            cpu->Reg[Rd] = cpu->Reg[Rn] - exOpr + ((cpu->CPSR >> 29) & 0x1) - 1;
            if(S_bit)CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], cpu->Reg[Rn] - !((cpu->CPSR >> 29) & 0x1), exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case RSC://arith
            cpu->Reg[Rd] = exOpr - cpu->Reg[Rn] + ((cpu->CPSR >> 29) & 0x1) - 1;
            if(S_bit)CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], exOpr - !((cpu->CPSR >> 29) & 0x1), cpu->Reg[Rn]);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case TST://logical
                //AND
            if(!S_bit)ArmPSRT(cpu,inst);
            else{CPSRUpdate(cpu, LOG, cpu->Reg[Rn] & exOpr, cpu->Reg[Rn], exOpr);};
            break;
        case TEQ://logical
                //EOR
            if(!S_bit)ArmPSRT(cpu,inst);
            else{CPSRUpdate(cpu, LOG, cpu->Reg[Rn] ^ exOpr, cpu->Reg[Rn], exOpr);};
            break;
        case CMP://arith
                //SUB
            if(!S_bit)ArmPSRT(cpu,inst);
            else{
                //printf("cmp\n");
                CPSRUpdate(cpu, A_SUB, cpu->Reg[Rn] - exOpr, cpu->Reg[Rn], exOpr);
            }
            //printf("CSPR:%08x\n", cpu->CPSR);
            break;
        case CMN://arith
            //ADD
            if(!S_bit)ArmPSRT(cpu,inst);
            else{CPSRUpdate(cpu, A_ADD, cpu->Reg[Rn] + exOpr, cpu->Reg[Rn], exOpr);};
            break;
        case ORR://logical
            cpu->Reg[Rd] = cpu->Reg[Rn] | exOpr;
            if(S_bit)CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case MOV://logical
            //printf("MOV Rd %d exOpr %08x\n", Rd, exOpr);
            cpu->Reg[Rd] = exOpr;
            if(S_bit)CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case BIC://logical
            cpu->Reg[Rd] = cpu->Reg[Rn] & ~(exOpr);
            if(S_bit)CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
        case MVN://logical
            cpu->Reg[Rd] = ~(exOpr);
            if(S_bit)CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
            if(Rd == PC)cpu->cycle += 2;
            break;
    }
    //cpu->cycle += 1;//1S
    //if(Rd == PC)cpu->cycle += 2;//1S+1N
}
//OK
void ArmBranch(Gba_Cpu *cpu, uint32_t inst){
    uint32_t offset;
    if((inst >> 23) & 0x1)offset = (inst << 2) | 0xff000000;
    else{offset = (uint32_t)(inst << 2) & 0x3ffffff;}

    if((inst >> 24) & 0x1){//Link Bit
        cpu->Reg[LR] = cpu->Reg[PC] - 0x4;//next instruction
    }
    //printf("old PC : %08x\n", cpu->Reg[PC]);
    cpu->Reg[PC] = cpu->Reg[PC] + (int32_t)offset;
    //printf("PC : %08x\n", cpu->Reg[PC]);
    cpu->fetchcache[1] = MemRead32(cpu, cpu->Reg[PC]);
    cpu->fetchcache[0] = MemRead32(cpu, cpu->Reg[PC] + 0x4);
    cpu->Reg[PC] = cpu->Reg[PC];
    cpu->Reg[PC] += 0x4;
    cpu->Reg[PC] = cpu->Reg[PC];

    cpu->cycle += 2;//2S+1N
}

void ArmBX(Gba_Cpu *cpu, uint32_t inst){
    uint8_t Rn = inst & 0xf;
    //if(Rn == PC)Undefined(cpu);//Exception
    //cpu->Reg[PC] = cpu->Reg[Rn];
    cpu->Reg[PC] = cpu->Reg[Rn] & 0xfffffffe;
    if(cpu->Reg[Rn] & 0x1 == 0x1){
        //printf("BX Thumb\n");
        cpu->fetchcache[1] = MemRead16(cpu, cpu->Reg[PC]);
        cpu->fetchcache[0] = MemRead16(cpu, cpu->Reg[PC] + 0x2);
        cpu->Reg[PC] = cpu->Reg[PC];
        cpu->Reg[PC] += 0x2;
        cpu->dMode = THUMB_MODE;
        cpu->CPSR |= 0x20;
    }
    else{
        cpu->fetchcache[1] = MemRead32(cpu, cpu->Reg[PC]);
        cpu->fetchcache[0] = MemRead32(cpu, cpu->Reg[PC] + 0x4);
        cpu->Reg[PC] = cpu->Reg[PC];
        cpu->Reg[PC] += 0x4;
        cpu->dMode = ARM_MODE;
        cpu->CPSR &= 0xffffffdf;
    }
    cpu->cycle += 2;//2S+1N
}
void ArmSWP(Gba_Cpu *cpu, uint32_t inst){
    uint8_t B_bit = (inst >> 22) & 0x1;
    uint32_t Rn = (inst >> 16) & 0xf;
    uint32_t Rd = (inst >> 12) & 0xf;
    uint32_t Rm = inst & 0xf;
    uint32_t tmp = 0;
    //MemWrite32(cpu, 0x2000000, 0xaaaaaaaa);
    if(B_bit){
        //byte
        cpu->Reg[Rd] = cpu->Reg[Rd] & 0xff;
        tmp = cpu->Reg[Rd];
        cpu->Reg[Rd] = (uint32_t)(MemRead8(cpu, cpu->Reg[Rn]) & 0xff);
        //printf("Rm %08x tmp %08x\n", cpu->Reg[Rm], tmp);
        if(Rd == Rm)MemWrite8(cpu, cpu->Reg[Rn], tmp);
        else{MemWrite8(cpu, cpu->Reg[Rn], cpu->Reg[Rm] & 0xff);}
        //printf("Content %08x\n", MemRead(cpu, cpu->Reg[Rn]));
    }
    else{
        //word
        tmp = cpu->Reg[Rd];
        cpu->Reg[Rd] = (uint32_t)(MemRead32(cpu, cpu->Reg[Rn]));
        //printf("Rm %08x tmp %08x\n", cpu->Reg[Rm], tmp);
        if(Rd == Rm)MemWrite32(cpu, cpu->Reg[Rn], tmp);
        else{MemWrite32(cpu, cpu->Reg[Rn], cpu->Reg[Rm]);}
        //printf("Content %08x\n", MemRead(cpu, cpu->Reg[Rn]));
    }
    cpu->cycle += 3;//1S+2N+1I
}
void ArmMUL(Gba_Cpu *cpu, uint32_t inst){
    uint8_t Acc = (inst >> 21) & 0x1;
    uint8_t S_bit = (inst >> 20) & 0x1;
    uint8_t Rd = (inst >> 16) & 0xf;
    uint8_t Rn = (inst >> 12) & 0xf;
    uint8_t Rs = (inst >> 8) & 0xf;
    uint8_t Rm = inst & 0xf;
    uint8_t m = 0;
    if((cpu->Reg[Rs] >> 8) & 0xffffff == 0x0 || (cpu->Reg[Rs] >> 8) & 0xffffff == 0xffffff)m = 1;
    else if((cpu->Reg[Rs] >> 16) & 0xffff == 0x0 || (cpu->Reg[Rs] >> 16) & 0xffff == 0xffff)m = 2;
    else if((cpu->Reg[Rs] >> 24) & 0xff == 0x0 || (cpu->Reg[Rs] >> 24) & 0xff == 0xff)m = 3;
    else{m = 4;}
    cpu->carry_out = 0;
    if(Acc){
        //MLA
        cpu->Reg[Rd] = (cpu->Reg[Rm] * cpu->Reg[Rs]) + cpu->Reg[Rn];
        if(S_bit)CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rm] * cpu->Reg[Rs], cpu->Reg[Rn]);
        cpu->cycle += (m + 1);
    }
    else{
        //MUL
        cpu->Reg[Rd] = (cpu->Reg[Rm] * cpu->Reg[Rs]);
        if(S_bit)CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rm], cpu->Reg[Rs]);
        cpu->cycle += m;
    }
}
void ArmMULL(Gba_Cpu *cpu, uint32_t inst){
    uint64_t opr;
    uint8_t U_bit = (inst >> 22) & 0x1;
    uint8_t Acc = (inst >> 21) & 0x1;
    uint8_t S_bit = (inst >> 20) & 0x1;
    uint8_t RdHi = (inst >> 16) & 0xf;
    uint8_t RdLo = (inst >> 12) & 0xf;
    uint8_t Rs = (inst >> 8) & 0xf;
    uint8_t Rm = inst & 0xf;
    uint8_t m = 0;

    if(Acc){
        //MLA
        if(U_bit==0){

            if((cpu->Reg[Rs] >> 8) & 0xffffff == 0x0)m = 1;
            else if((cpu->Reg[Rs] >> 16) & 0xffff == 0x0)m = 2;
            else if((cpu->Reg[Rs] >> 24) & 0xff == 0x0)m = 3;
            else{m = 4;}

            opr = (((uint64_t)cpu->Reg[RdHi]) << 32) + (uint64_t)cpu->Reg[RdLo];
            cpu->Reg[RdHi] = (uint32_t)(((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) + opr) & 0xffffffff00000000) >> 32);
            cpu->Reg[RdLo] = (uint32_t)(((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) + opr) & 0xffffffff));
        }
        else{

            if((cpu->Reg[Rs] >> 8) & 0xffffff == 0x0 || (cpu->Reg[Rs] >> 8) & 0xffffff == 0xffffff)m = 1;
            else if((cpu->Reg[Rs] >> 16) & 0xffff == 0x0 || (cpu->Reg[Rs] >> 16) & 0xffff == 0xffff)m = 2;
            else if((cpu->Reg[Rs] >> 24) & 0xff == 0x0 || (cpu->Reg[Rs] >> 24) & 0xff == 0xff)m = 3;
            else{m = 4;}

            opr = (((int64_t)(int32_t)cpu->Reg[RdHi]) << 32) + (int64_t)(int32_t)cpu->Reg[RdLo];
            //printf("opr:%08x\n", opr);
            cpu->Reg[RdHi] = (uint32_t)(((((int64_t)(int32_t)cpu->Reg[Rm] * (int64_t)(int32_t)cpu->Reg[Rs]) + opr) & 0xffffffff00000000) >> 32);
            cpu->Reg[RdLo] = (uint32_t)(((((int64_t)cpu->Reg[Rm] * (int64_t)cpu->Reg[Rs]) + opr) & 0xffffffff));
        }
        if(S_bit){
            if((cpu->Reg[RdHi] >> 31) & 0x1)cpu->CPSR = cpu->CPSR | 0x80000000;
            else{cpu->CPSR = cpu->CPSR & 0x7fffffff;}

            if(cpu->Reg[RdHi] == 0 && cpu->Reg[RdLo] == 0)cpu->CPSR = cpu->CPSR | 0x40000000;
            else{cpu->CPSR = cpu->CPSR & 0xbfffffff;}
        }
        cpu->cycle += (m+2);
    }
    else{
        //MUL
        if(U_bit==0){

            if((cpu->Reg[Rs] >> 8) & 0xffffff == 0x0)m = 1;
            else if((cpu->Reg[Rs] >> 16) & 0xffff == 0x0)m = 2;
            else if((cpu->Reg[Rs] >> 24) & 0xff == 0x0)m = 3;
            else{m = 4;}

            cpu->Reg[RdHi] = (uint32_t)((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) & 0xffffffff00000000) >> 32);
            cpu->Reg[RdLo] = (uint32_t)((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) & 0xffffffff));
        }
        else{

            if((cpu->Reg[Rs] >> 8) & 0xffffff == 0x0 || (cpu->Reg[Rs] >> 8) & 0xffffff == 0xffffff)m = 1;
            else if((cpu->Reg[Rs] >> 16) & 0xffff == 0x0 || (cpu->Reg[Rs] >> 16) & 0xffff == 0xffff)m = 2;
            else if((cpu->Reg[Rs] >> 24) & 0xff == 0x0 || (cpu->Reg[Rs] >> 24) & 0xff == 0xff)m = 3;
            else{m = 4;}

            cpu->Reg[RdHi] = (uint32_t)((((int64_t)(int32_t)cpu->Reg[Rm] * (int64_t)(int32_t)cpu->Reg[Rs]) & 0xffffffff00000000) >> 32);
            cpu->Reg[RdLo] = (uint32_t)((((int64_t)cpu->Reg[Rm] * (int64_t)cpu->Reg[Rs]) & 0xffffffff));
        }
        if(S_bit){
            if((cpu->Reg[RdHi] >> 31) & 0x1)cpu->CPSR = cpu->CPSR | 0x80000000;
            else{cpu->CPSR = cpu->CPSR & 0x7fffffff;}

            if(cpu->Reg[RdHi] == 0 && cpu->Reg[RdLo] == 0)cpu->CPSR = cpu->CPSR | 0x40000000;
            else{cpu->CPSR = cpu->CPSR & 0xbfffffff;}
        }
        cpu->cycle += (m+1);
    }
}
void ArmSWI(Gba_Cpu *cpu, uint32_t inst){}
void ArmSDT(Gba_Cpu *cpu, uint32_t inst){
    uint8_t Imm = (inst >> 25) & 0x1;
    uint8_t P_bit = (inst >> 24) & 0x1;
    uint8_t U_bit = (inst >> 23) & 0x1;
    uint8_t B_bit = (inst >> 22) & 0x1;
    uint8_t W_bit = (inst >> 21) & 0x1;
    uint8_t L_bit = (inst >> 20) & 0x1;
    uint8_t Rn = (inst >> 16) & 0xf;
    uint8_t Rd = (inst >> 12) & 0xf;
    uint16_t Offset = (inst) & 0xfff;
    uint32_t Opr;
    //printf("Offset : %d, Imm:%d\n", Offset, Imm);
    if(Imm)Opr = cpu->Reg[(Offset & 0xf)];
    else{Opr = Offset;}
    if(!U_bit)Opr = 0 - Opr;
    if(P_bit && W_bit)cpu->Reg[Rn] += Opr;
    //printf("SDT Opr:%08x\n", Opr);
    if(L_bit){
        //LDR
        if(B_bit){
            //printf("B-bit\n");
            cpu->Reg[Rd] = cpu->Reg[Rd] & 0xffffff00;
            switch((cpu->Reg[Rn] + Opr) % 4){
                case 0:
                    cpu->Reg[Rd] = MemRead8(cpu, cpu->Reg[Rn] + Opr) & 0xff;
                    break;
                case 1:
                    cpu->Reg[Rd] = (MemRead8(cpu, cpu->Reg[Rn] + Opr) >> 8) & 0xff;
                    break;
                case 2:
                    cpu->Reg[Rd] = (MemRead8(cpu, cpu->Reg[Rn] + Opr) >> 16) & 0xff;
                    break;
                case 3:
                    cpu->Reg[Rd] = (MemRead8(cpu, cpu->Reg[Rn] + Opr) >> 24) & 0xff;
                    break;
            }
        }
        else{
            if((cpu->Reg[Rn] + Opr) % 4){
                printf("Address Must Be Multiply with 4!!!\n");
                //exit();
            }
            else{
                //printf("LDR Rn:%d, Rn Content:%08x, Opr:%08x\n", Rn, cpu->Reg[Rn], Opr);
                cpu->Reg[Rd] = MemRead32(cpu, cpu->Reg[Rn] + Opr);
            }
        }
        cpu->cycle += 2;//1S+1N+1I
        if(Rd == PC)cpu->cycle += 2;
    }
    else{
        //STR
        //if(P_bit && W_bit)cpu->Reg[Rn] += Opr;
        if(B_bit){
            MemWrite8(cpu, cpu->Reg[Rn] + Opr, cpu->Reg[Rd] & 0xff | (cpu->Reg[Rd] & 0xff) << 8 | (cpu->Reg[Rd] & 0xff) << 16 | (cpu->Reg[Rd] & 0xff) << 24);
        }
        else{
            MemWrite32(cpu, cpu->Reg[Rn] + Opr, cpu->Reg[Rd]);
        }
        cpu->cycle += 1;//2N
    }
    if(!P_bit)cpu->Reg[Rn] += Opr;
}
void ArmSDTS(Gba_Cpu *cpu, uint32_t inst){
    uint8_t Imm = (inst >> 22) & 0x1;
    uint8_t P_bit = (inst >> 24) & 0x1;
    uint8_t U_bit = (inst >> 23) & 0x1;
    //uint8_t B_bit = (inst >> 22) & 0x1;
    uint8_t W_bit = (inst >> 21) & 0x1;
    uint8_t L_bit = (inst >> 20) & 0x1;
    uint8_t Rn = (inst >> 16) & 0xf;
    uint8_t Rd = (inst >> 12) & 0xf;
    uint8_t SH = (inst >> 5) & 0x3;
    uint8_t Rm = inst & 0xf;
    uint32_t Opr;
    //printf("Offset : %d, Imm:%d\n", Offset, Imm);
    if(Imm){Opr = (((inst >> 8) & 0xf) << 4) | (inst & 0xf);}
    else{Opr = cpu->Reg[Rm];}
    if(!U_bit)Opr = 0 - Opr;
    if(P_bit && W_bit)cpu->Reg[Rn] += Opr;
    if(L_bit){
        //LDR
        //MemWrite(cpu, cpu->Reg[Rn] + Opr, 0xaaaaaaaa);
        if(SH == 2)cpu->Reg[Rd] = cpu->Reg[Rd] & 0x00000000;
        else{cpu->Reg[Rd] = cpu->Reg[Rd] & 0x00000000;}
        //printf("instruction:%08x\n", inst);
        //printf("mode:%d, SH:%d, addr:%08x\n", (cpu->Reg[Rn] + Opr) % 4, SH, cpu->Reg[Rn] + Opr);
        switch((cpu->Reg[Rn] + Opr) % 4){
            case 0:
                if(SH==2)cpu->Reg[Rd] = MemRead8(cpu, cpu->Reg[Rn] + Opr);
                else{cpu->Reg[Rd] = MemRead16(cpu, cpu->Reg[Rn] + Opr);}
                break;
            case 1:
                if(SH==2)cpu->Reg[Rd] = (MemRead8(cpu, cpu->Reg[Rn] + Opr));
                break;
            case 2:
                if(SH==2)cpu->Reg[Rd] = (MemRead8(cpu, cpu->Reg[Rn] + Opr));
                else{cpu->Reg[Rd] = (MemRead16(cpu, cpu->Reg[Rn] + Opr));}
                break;
            case 3:
                if(SH==2)cpu->Reg[Rd] = (MemRead8(cpu, cpu->Reg[Rn] + Opr));
                break;
        }
        if(SH == 2)cpu->Reg[Rd] = (uint32_t)(((int32_t)(cpu->Reg[Rd] << 24)) >> 24);
        else if(SH == 3)cpu->Reg[Rd] = (uint32_t)(((int32_t)(cpu->Reg[Rd] << 16)) >> 16);
        cpu->cycle += 2;
        if(Rd == PC)cpu->cycle += 2;
    }
    else{
        //STR
        //printf("STRH\n");
        if(SH == 1){
            MemWrite16(cpu, cpu->Reg[Rn] + Opr, ((cpu->Reg[Rd] & 0xffff) | ((cpu->Reg[Rd] & 0xffff) << 16)));
            //printf("Rd:%08x, addr:%08x, value:%08x\n", cpu->Reg[Rd], cpu->Reg[Rn] + Opr, MemRead(cpu, cpu->Reg[Rn] + Opr));
        }
        cpu->cycle += 1;
    }
    if(!P_bit)cpu->Reg[Rn] += Opr;
}
void ArmUDF(Gba_Cpu *cpu, uint32_t inst){}
void ArmBDT(Gba_Cpu *cpu, uint32_t inst){
    uint8_t P_bit = (inst >> 24) & 0x1;
    uint8_t U_bit = (inst >> 23) & 0x1;
    uint8_t S_bit = (inst >> 22) & 0x1;
    uint8_t W_bit = (inst >> 21) & 0x1;
    uint8_t L_bit = (inst >> 20) & 0x1;
    uint32_t Rn = (inst >> 16) & 0xf;
    uint16_t RegList = inst & 0xffff;
    uint8_t shift = 0;
    uint32_t RWaddr = cpu->Reg[Rn];
    uint8_t n = 0;
    //printf("BDT\n");
    if(L_bit){
        //LDM
        while(shift < 16){
            if(U_bit){
                //printf("U-bit\n");
                if((RegList >> shift) & 0x1){
                    if(P_bit)RWaddr += 4;
                    cpu->Reg[shift] = MemRead32(cpu, RWaddr);
                    if(!P_bit)RWaddr += 4;
                    n += 1;
                }
            }
            else{
                //printf("in U=0, %d\n", 15-shift);
                if((RegList >> (15 - shift)) & 0x1){
                    if(P_bit)RWaddr -= 4;
                    cpu->Reg[15 - shift] = MemRead32(cpu, RWaddr);
                    if(!P_bit)RWaddr -= 4;
                    n += 1;
                }
            }
            shift+=1;
        }
        if(W_bit)cpu->Reg[Rn] = RWaddr;
        cpu->cycle += (n+1);
        if(Rn == PC)cpu->cycle += 2;
    }
    else{
        //STM
        while(shift < 16){
            if(U_bit){
                if((RegList >> shift) & 0x1){
                    if(P_bit)RWaddr += 4;
                    MemWrite32(cpu, RWaddr, cpu->Reg[shift]);
                    if(!P_bit)RWaddr += 4;
                    n += 1;
                }
            }
            else{
                if((RegList >> (15 - shift)) & 0x1){
                    if(P_bit)RWaddr -= 4;
                    MemWrite32(cpu, RWaddr, cpu->Reg[15 - shift]);
                    if(!P_bit)RWaddr -= 4;
                    n += 1;
                }
            }
            shift+=1;
        }
        if(W_bit)cpu->Reg[Rn] = RWaddr;
        cpu->cycle += (n);
    }
}