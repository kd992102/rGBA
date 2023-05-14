#include "arm_instruction.h"

uint32_t BarrelShifter(Gba_Cpu *cpu, uint32_t Opr, uint8_t Imm, uint32_t result){
    uint8_t shift_type;
    uint8_t shift;
    if(Imm){
        shift = (((Opr >> 8) & 0xf) << 1);
        cpu->carry_out = ((Opr & 0xff) >> (shift - 1)) & 0x1;
        result = ((Opr & 0xff) << (32 - shift)) | (Opr >> shift);
    }
    else{
        shift_type = (Opr >> 5) & 0x3;
        if((Opr >> 4) & 0x1){
            shift = cpu->Reg[(Opr >> 8) & 0xf] & 0xf;
            if(!shift)cpu->carry_out = (cpu->CPSR >> 29) & 0x1;
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
                if(shift == 1){
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

void CPSRupdate(Gba_Cpu *cpu, uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB){
        printf("A:%08x, B:%08x, res:%08x\n",parameterA, parameterB,result);
    if((result >> 31))cpu->CPSR = cpu->CPSR | 0x80000000;//N flag
    else{cpu->CPSR = cpu->CPSR & 0x7fffffff;}

    if(!result)cpu->CPSR = cpu->CPSR | 0x40000000;//Z flag
    else{cpu->CPSR = cpu->CPSR & 0xbfffffff;}
        
    if(Opcode == SUB || Opcode == RSB || Opcode == ADD || Opcode == ADC || Opcode == SBC || Opcode == RSC || Opcode == CMP || Opcode == CMN){//Arithmetic
            //check V flag
        if(Opcode == ADD || Opcode == ADC || Opcode == CMN){
            if((parameterA >> 31) == (parameterB >> 31) && (parameterA >> 31) != (result >> 31)){
                   cpu->CPSR = cpu->CPSR | 0x10000000;//sign change
            }
            else{cpu->CPSR = cpu->CPSR & 0xefffffff;}
            
            if((result < parameterA) && (result < parameterB))cpu->CPSR = cpu->CPSR | 0x20000000;
            else{cpu->CPSR = cpu->CPSR & 0xdfffffff;}
        }
        else if(Opcode == SUB || Opcode == SBC || Opcode == CMP){
            if((parameterA >> 31) != (parameterB >> 31) && (result >> 31) != (parameterA >> 31)){
                cpu->CPSR = cpu->CPSR | 0x10000000;//sign change
            }
            else{cpu->CPSR & 0xefffffff;}
                
            if((result < parameterA) && (result < parameterB) && result != 0)cpu->CPSR = cpu->CPSR & 0xdfffffff;
            else{cpu->CPSR = cpu->CPSR | 0x20000000;}
        }
        else if(Opcode == RSB || Opcode == RSC){
            if((parameterB >> 31) != (parameterA >> 31) && (result >> 31) != (parameterB >> 31)){
                cpu->CPSR = cpu->CPSR | 0x10000000;//sign change
            }
            else{cpu->CPSR = cpu->CPSR & 0xefffffff;}

            if((result < parameterA) && (result < parameterB))cpu->CPSR = cpu->CPSR & 0xdfffffff;
            else{cpu->CPSR = cpu->CPSR | 0x20000000;}
        }
    }
    else{
        if(cpu->carry_out)cpu->CPSR = cpu->CPSR | 0x20000000;
        else{cpu->CPSR = cpu->CPSR & 0xdfffffff;}
           //V unaffected
    }
}

void PSRTransfer(Gba_Cpu *cpu, uint32_t inst){
    uint8_t psr = (inst >> 22) & 0x1;
    uint8_t bit = (inst >> 21) & 0x1;
    uint8_t Imm = (inst >> 25) & 0x1;
    uint8_t b_only = (inst >> 16) & 0x1;
    uint8_t Rd = (inst >> 12) & 0xf;
    uint8_t Rm = (inst & 0xf);
    uint32_t Opr = (inst & 0xfff);
    uint32_t res;
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
        if(psr)cpu->Reg[Rd] = cpu->SPSR;
        else{cpu->Reg[Rd] = cpu->CPSR;}
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
    if(1){
        switch(Opcode){
            case AND://logical
                cpu->Reg[Rd] = cpu->Reg[Rn] & exOpr;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case EOR://logical
                cpu->Reg[Rd] = cpu->Reg[Rn] ^ exOpr;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case SUB://arith
                cpu->Reg[Rd] = cpu->Reg[Rn] - exOpr;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case RSB://arith
                cpu->Reg[Rd] = exOpr - cpu->Reg[Rn];
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case ADD://arith
                cpu->Reg[Rd] = cpu->Reg[Rn] + exOpr;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case ADC://arith
                cpu->Reg[Rd] = cpu->Reg[Rn] + exOpr + ((cpu->CPSR >> 29) & 0x1);
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case SBC://arith
                cpu->Reg[Rd] = cpu->Reg[Rn] - exOpr + ((cpu->CPSR >> 29) & 0x1) - 1;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case RSC://arith
                cpu->Reg[Rd] = exOpr - cpu->Reg[Rn] + ((cpu->CPSR >> 29) & 0x1) - 1;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case TST://logical
                //AND
                if(!S_bit)PSRTransfer(cpu,inst);
                else{CPSRupdate(cpu, Opcode, cpu->Reg[Rn] & exOpr, cpu->Reg[Rn], exOpr);};
                break;
            case TEQ://logical
                //EOR
                if(!S_bit)PSRTransfer(cpu,inst);
                else{CPSRupdate(cpu, Opcode, cpu->Reg[Rn] ^ exOpr, cpu->Reg[Rn], exOpr);};
                break;
            case CMP://arith
                //SUB
                if(!S_bit)PSRTransfer(cpu,inst);
                else{CPSRupdate(cpu, Opcode, cpu->Reg[Rn] - exOpr, cpu->Reg[Rn], exOpr);};
                break;
            case CMN://arith
                //ADD
                if(!S_bit)PSRTransfer(cpu,inst);
                else{CPSRupdate(cpu, Opcode, cpu->Reg[Rn] + exOpr, cpu->Reg[Rn], exOpr);};
                break;
            case ORR://logical
                cpu->Reg[Rd] = cpu->Reg[Rn] | exOpr;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case MOV://logical
                cpu->Reg[Rd] = exOpr;
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case BIC://logical
                cpu->Reg[Rd] = cpu->Reg[Rn] & ~(exOpr);
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
            case MVN://logical
                cpu->Reg[Rd] = ~(exOpr);
                if(S_bit)CPSRupdate(cpu, Opcode, cpu->Reg[Rd], cpu->Reg[Rn], exOpr);
                break;
        }
    }
}
//OK
void ArmBranch(Gba_Cpu *cpu, uint32_t inst){
    uint32_t offset;
    if((inst >> 23) & 0x1)offset = (inst << 2) | 0xff000000;
    else{offset = (uint32_t)(inst << 2) & 0x3ffffff;}
    if(CheckCond(cpu)){
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
void ArmMUL(Gba_Cpu *cpu, uint32_t inst){
    uint8_t Acc = (inst >> 21) & 0x1;
    uint8_t S_bit = (inst >> 20) & 0x1;
    uint8_t Rd = (inst >> 16) & 0xf;
    uint8_t Rn = (inst >> 12) & 0xf;
    uint8_t Rs = (inst >> 8) & 0xf;
    uint8_t Rm = inst & 0xf;
    uint8_t cpu->carry_out = 0;
    if(1){
        if(Acc){
            //MLA
            cpu->Reg[Rd] = (cpu->Reg[Rm] * cpu->Reg[Rs]) + cpu->Reg[Rn];
            if(S_bit)CPSRupdate(cpu, ADD, cpu->Reg[Rd], cpu->Reg[Rm] * cpu->Reg[Rs], cpu->Reg[Rn]);
        }
        else{
            //MUL
            cpu->Reg[Rd] = (cpu->Reg[Rm] * cpu->Reg[Rs]);
            if(S_bit)CPSRupdate(cpu, ADD, cpu->Reg[Rd], cpu->Reg[Rm], cpu->Reg[Rs]);
        }
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
    if(1){
        if(Acc){
            //MLA
            if(U_bit==0){
                opr = (((uint64_t)cpu->Reg[RdHi]) << 32) + (uint64_t)cpu->Reg[RdLo];
                cpu->Reg[RdHi] = (uint32_t)(((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) + opr) & 0xffffffff00000000) >> 32);
                cpu->Reg[RdLo] = (uint32_t)(((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) + opr) & 0xffffffff));
            }
            else{
                opr = (((int64_t)(int32_t)cpu->Reg[RdHi]) << 32) + (int64_t)(int32_t)cpu->Reg[RdLo];
                cpu->Reg[RdHi] = (uint32_t)(((((int64_t)(int32_t)cpu->Reg[Rm] * (int64_t)(int32_t)cpu->Reg[Rs]) + opr) & 0xffffffff00000000) >> 32);
                cpu->Reg[RdLo] = (uint32_t)(((((int64_t)cpu->Reg[Rm] * (int64_t)cpu->Reg[Rs]) + opr) & 0xffffffff));
            }
            if(S_bit){
                if((cpu->Reg[RdHi] >> 31) & 0x1)cpu->CPSR = cpu->CPSR | 0x80000000;
                else{cpu->CPSR = cpu->CPSR & 0x7fffffff;}

                if(cpu->Reg[RdHi] == 0 && cpu->Reg[RdLo] == 0)cpu->CPSR = cpu->CPSR | 0x40000000;
                else{cpu->CPSR = cpu->CPSR & 0xbfffffff;}
            }
        }
        else{
            //MUL
            if(U_bit==0){
                cpu->Reg[RdHi] = (uint32_t)((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) & 0xffffffff00000000) >> 32);
                cpu->Reg[RdLo] = (uint32_t)((((uint64_t)cpu->Reg[Rm] * (uint64_t)cpu->Reg[Rs]) & 0xffffffff));
            }
            else{
                cpu->Reg[RdHi] = (uint32_t)((((int64_t)(int32_t)cpu->Reg[Rm] * (int64_t)(int32_t)cpu->Reg[Rs]) & 0xffffffff00000000) >> 32);
                cpu->Reg[RdLo] = (uint32_t)((((int64_t)cpu->Reg[Rm] * (int64_t)cpu->Reg[Rs]) & 0xffffffff));
            }
            if(S_bit){
                if((cpu->Reg[RdHi] >> 31) & 0x1)cpu->CPSR = cpu->CPSR | 0x80000000;
                else{cpu->CPSR = cpu->CPSR & 0x7fffffff;}

                if(cpu->Reg[RdHi] == 0 && cpu->Reg[RdLo] == 0)cpu->CPSR = cpu->CPSR | 0x40000000;
                else{cpu->CPSR = cpu->CPSR & 0xbfffffff;}
            }
        }
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
    uint8_t Offset = (inst) & 0xfff;
}
void ArmSDTS(Gba_Cpu *cpu, uint32_t inst){}
void ArmUDF(Gba_Cpu *cpu, uint32_t inst){}
void ArmBDT(Gba_Cpu *cpu, uint32_t inst){}