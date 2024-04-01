#include <stdio.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
#include "thumb_instruction.h"

extern Gba_Cpu *cpu;
extern GbaMem *Mem;

void ThumbMULLS(uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Rb = (inst >> 8) & 0x7;
    uint8_t RegList = (inst) & 0xff;
    uint8_t n = 0;
    cpu->DebugFunc = 25;
    if(L_bit){
        for(int i=0;i<8;i++){
            if((RegList >> i) & 0x1){
                cpu->Reg[i] = MemRead32(cpu->Reg[Rb]);
                cpu->Reg[Rb] = cpu->Reg[Rb] + 4;
                n += 1;
            }
        }
        cpu->cycle += (n + 1);
    }
    else{
        for(int i=0;i<8;i++){
            if((RegList >> i) & 0x1){
                MemWrite32(cpu->Reg[Rb], cpu->Reg[i]);
                cpu->Reg[Rb] = cpu->Reg[Rb] + 4;
                n += 1;
            }
        }
        cpu->cycle += (n);
    }
}
void ThumbCondB(uint16_t inst){
    uint8_t cond = (inst >> 8) & 0xf;
    int8_t Offset = inst & 0xff;
    cpu->DebugFunc = 26;
    cpu->Cond = cond;
    if(CheckCond(cpu)){
        cpu->Reg[PC] = cpu->Reg[PC] + ((int8_t)Offset) * 2;
        if(((Offset >> 7) & 0x1) == 0x1){
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
        else{
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
        cpu->cycle += 2;
    }
    else{
    }
}
void ThumbSWI(uint16_t inst){
    //printf("Thumb SWI\n");
    uint32_t swi_num = inst & 0xff;
    cpu->DebugFunc = 27;
    RecoverReg(cpu->Cmode);
    cpu->saveMode = THUMB_MODE;
    cpu->dMode = ARM_MODE;
    cpu->CPSR = (cpu->CPSR & 0xffffff00) + 0x13;
    cpu->Cmode = ChkCPUMode();
    //printf("PC->%x\n", cpu->Reg[PC]);
    cpu->Reg[LR] = cpu->Reg[PC] - 0x4 + 0x2;
    //Initial SWI address
    cpu->Reg[PC] = 0x08;
    cpu->SPSR = cpu->CPSR;
    //Entry address offset
    //cpu->Reg[PC] += swi_num;
    cpu->fetchcache[1] = MemRead32(cpu->Reg[PC]);
    cpu->fetchcache[0] = MemRead32(cpu->Reg[PC] + 0x4);
    cpu->Reg[PC] += 0x4;
    cpu->cycle += 2;//2S+1N
}
void ThumbUCOND(uint16_t inst){
    uint32_t Offset = inst & 0x7ff;
    cpu->DebugFunc = 28;
    Offset = Offset << 1;
    if(((Offset >> 11) & 0x1))Offset = Offset | 0xfffff000;
    cpu->Reg[PC] = cpu->Reg[PC] + Offset;
    cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
    cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
    cpu->Reg[PC] = cpu->Reg[PC];
    cpu->Reg[PC] += 0x2;
    cpu->cycle += 2;
}
void ThumbLONGBL(uint16_t inst){
    uint8_t H = (inst >> 11) & 0x1;
    uint32_t Offset = ((inst) & 0x7ff);
    uint32_t tmp = 0;
    cpu->DebugFunc = 29;
    if(H == 0){
        Offset = Offset << 12;
        if((Offset >> 22) & 0x1 == 1)cpu->Reg[LR] = cpu->Reg[PC] - (((~Offset) & 0x3fffff) + 1);
        else{
            cpu->Reg[LR] = cpu->Reg[PC] + Offset;
        }
    }
    else{
        Offset = Offset << 1;
        tmp = cpu->Reg[PC] - 0x2;
        cpu->Reg[PC] = cpu->Reg[LR] + (Offset);
        cpu->Reg[LR] = tmp | 0x1;

        cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
        cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
        cpu->Reg[PC] = cpu->Reg[PC];
        cpu->Reg[PC] += 2;
        cpu->cycle += 2;
    }
}
void ThumbLSH(uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Offset = (inst >> 6) & 0x1f;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    uint32_t addr = cpu->Reg[Rb] + (Offset << 1);
    cpu->DebugFunc = 20;
    if(L_bit){
        //LDR
        cpu->Reg[Rd] = MemRead16(cpu->Reg[Rb] + (Offset << 1));
        cpu->cycle += 2;
        if(Rd == PC){
            cpu->cycle += 2;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
    }
    else{
        MemWrite16(cpu->Reg[Rb] + (Offset << 1), (uint16_t)(cpu->Reg[Rd] & 0xffff));
        cpu->cycle += 1;
    }
}
void ThumbSPLS(uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint16_t Word = (inst) & 0xff;
    cpu->DebugFunc = 21;
    if(L_bit){
        //LDR
        cpu->Reg[Rd] = MemRead32(cpu->Reg[SP] + (Word << 2));
        cpu->cycle += 2;
        if(Rd == PC){
            cpu->cycle += 2;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
    }
    else{
        MemWrite32(cpu->Reg[SP] + (Word << 2), cpu->Reg[Rd]);
        cpu->cycle += 1;
    }
}
void ThumbLADDR(uint16_t inst){
    uint8_t SP_bit = (inst >> 11) & 0x1;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint16_t Word = (inst) & 0xff;
    cpu->DebugFunc = 22;
    if(SP_bit){
        //
        cpu->Reg[Rd] = cpu->Reg[SP] + (Word << 2);
    }
    else{
        //PC bit 1 force to 0
        cpu->Reg[Rd] = (cpu->Reg[PC] & 0xfffffffd) + (Word << 2);
    }
    if(Rd == PC){
        cpu->cycle += 2;
        cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
        cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
        cpu->Reg[PC] += 0x2;
    }
}
void ThumbADDSP(uint16_t inst){
    uint8_t S_bit = (inst >> 7) & 0x1;
    uint8_t Word = (inst) & 0x7f;
    cpu->DebugFunc = 23;
    if(S_bit){
        //
        cpu->Reg[SP] = cpu->Reg[SP] - (Word << 2);
    }
    else{
        //PC bit 1 force to 0
        cpu->Reg[SP] = cpu->Reg[SP] + (Word << 2);
    }
}
void ThumbPPREG(uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t R_bit = (inst >> 8) & 0x1;
    uint8_t RegList = (inst) & 0xff;
    uint8_t n = 0;
    cpu->DebugFunc = 24;
    if(L_bit){
        //POP
        for(int i=0;i<8;i++){
            if((RegList >> i) & 0x1){
                cpu->Reg[i] = MemRead32(cpu->Reg[SP]);
                cpu->Reg[SP] = cpu->Reg[SP] + 4;
                n += 1;
            }
        }
        if(R_bit){
            cpu->Reg[PC] = MemRead32(cpu->Reg[SP]) & 0xfffffffe;
            cpu->Reg[SP] = cpu->Reg[SP] + 4;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 2;
            cpu->cycle += 3;
        }
        cpu->cycle += (n + 1);
    }
    else{
        //PUSH
        if(R_bit){
                cpu->Reg[SP] = cpu->Reg[SP] - 4;
                MemWrite32(cpu->Reg[SP], cpu->Reg[LR]);
                n += 1;
        }
        for(int i=7;i>=0;i--){
            if((RegList >> i) & 0x1){
                cpu->Reg[SP] = cpu->Reg[SP] - 4;
                MemWrite32(cpu->Reg[SP], cpu->Reg[i]);
                n += 1;
            }
        }
        cpu->cycle += (n);
    }
}
void ThumbALU(uint16_t inst){
    uint8_t Op = (inst >> 6) & 0xf;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    uint8_t m = 0;
    uint32_t tmp;
    cpu->DebugFunc = 15;
    tmp = cpu->Reg[Rd];
    switch(Op){
        case 0:
            //AND
            cpu->Reg[Rd] = (cpu->Reg[Rd] & cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            break;
        case 1:
            //EOR
            cpu->Reg[Rd] = (cpu->Reg[Rd] ^ cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            break;
        case 2:
            //LOL
            cpu->Reg[Rd] = (cpu->Reg[Rd] << cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            cpu->cycle += 1;
            break;
        case 3:
            //LOR
            cpu->Reg[Rd] = (cpu->Reg[Rd] >> cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            cpu->cycle += 1;
            break;
        case 4:
            //ASR
            cpu->Reg[Rd] = ((int32_t)(cpu->Reg[Rd]) >> (cpu->Reg[Rs]));
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            cpu->cycle += 1;
            break;
        case 5:
            //ADC
            cpu->Reg[Rd] = (cpu->Reg[Rd]) + (cpu->Reg[Rs]) + ((cpu->CPSR >> 29) & 0x1);
            CPSRUpdate(A_ADD, cpu->Reg[Rd], tmp + ((cpu->CPSR >> 29) & 0x1), cpu->Reg[Rs]);
            break;
        case 6:
            //SBC
            cpu->Reg[Rd] = (cpu->Reg[Rd]) - (cpu->Reg[Rs]) - !((cpu->CPSR >> 29) & 0x1);
            CPSRUpdate(A_SUB, cpu->Reg[Rd], tmp - !((cpu->CPSR >> 29) & 0x1), cpu->Reg[Rs]);
            break;
        case 7:
            //ROR
            cpu->Reg[Rd] = ((cpu->Reg[Rd]) << (32 - cpu->Reg[Rs])) | (cpu->Reg[Rd] >> cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, cpu->Reg[Rs]);
            cpu->cycle += 1;
            break;
        case 8:
            //TST
            CPSRUpdate(LOG, (cpu->Reg[Rd] & cpu->Reg[Rs]), (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 9:
            //NEG
            cpu->Reg[Rd] = (0 - (cpu->Reg[Rs]));
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            break;
        case 10:
            //CMP
            CPSRUpdate(A_SUB, cpu->Reg[Rd] - cpu->Reg[Rs], cpu->Reg[Rd], cpu->Reg[Rs]);
            break;
        case 11:
            //CMN
            CPSRUpdate(A_ADD, (cpu->Reg[Rd]) + (cpu->Reg[Rs]), (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 12:
            //ORR
            cpu->Reg[Rd] = (cpu->Reg[Rd] | cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            break;
        case 13:
            //MUL
            
            if(((tmp >> 8) & 0xffffff) == 0x0 || ((tmp >> 8) & 0xffffff) == 0xffffff)m = 1;
            else if(((tmp >> 16) & 0xffff) == 0x0 || ((tmp >> 16) & 0xffff) == 0xffff)m = 2;
            else if(((tmp >> 24) & 0xff) == 0x0 || ((tmp >> 24) & 0xff) == 0xff)m = 3;
            else{m = 4;}
            cpu->Reg[Rd] = (cpu->Reg[Rd]) * (cpu->Reg[Rs]);
            CPSRUpdate(A_ADD, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            cpu->cycle += m;
            break;
        case 14:
            //BIC
            cpu->Reg[Rd] = (cpu->Reg[Rd]) & ~(cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            break;
        case 15:
            //MVN
            cpu->Reg[Rd] = ~(cpu->Reg[Rs]);
            CPSRUpdate(LOG, cpu->Reg[Rd], tmp, (cpu->Reg[Rs]));
            break;
    }
    if(Rd == PC)cpu->cycle += 2;
}
void ThumbBX(uint16_t inst){
    uint8_t Op = (inst >> 8) & 0x3;
    uint8_t H1 = (inst >> 7) & 0x1;
    uint8_t H2 = (inst >> 6) & 0x1;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    cpu->DebugFunc = 16;
    if(H2)Rs += 8;
    if(H1)Rd += 8;
    switch(Op){
        case 0:
            cpu->Reg[Rd] += cpu->Reg[Rs];
            break;
        case 1:
            CPSRUpdate(A_SUB, cpu->Reg[Rd] - cpu->Reg[Rs], cpu->Reg[Rd], cpu->Reg[Rs]);
            break;
        case 2:
            cpu->Reg[Rd] = cpu->Reg[Rs];
            break;
        case 3:
            //printf("Rs:%d, Rs Content:%08x\n", Rs, cpu->Reg[Rs]);
            cpu->Reg[PC] = cpu->Reg[Rs];
            if(cpu->Reg[PC] & 0x1){
                cpu->Reg[PC] &= 0xfffffffe;
                cpu->dMode = THUMB_MODE;
                cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
                cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
                cpu->Reg[PC] = cpu->Reg[PC];
                cpu->Reg[PC] += 0x2;
                cpu->dMode = THUMB_MODE;
                cpu->CPSR |= 0x20;
            }
            else{
                cpu->Reg[PC] &= 0xfffffffe;
                cpu->dMode = ARM_MODE;
                cpu->fetchcache[1] = MemRead32(cpu->Reg[PC]);
                cpu->fetchcache[0] = MemRead32(cpu->Reg[PC] + 0x4);
                cpu->Reg[PC] = cpu->Reg[PC];
                cpu->Reg[PC] += 0x4;
                cpu->dMode = ARM_MODE;
                cpu->CPSR &= 0xffffffdf;
            }
            cpu->cycle += 2;
            break;
    }
    if(Rd == PC)cpu->cycle += 2;
}
void ThumbPCLOAD(uint16_t inst){
    uint8_t Rd = (inst >> 8) & 0x7;
    uint8_t Word = inst & 0xff;
    cpu->DebugFunc = 17;
    cpu->Reg[Rd] = MemRead32((cpu->Reg[PC] & 0xfffffffd) + (Word << 2));
    cpu->cycle += 2;
    if(Rd == PC){
            cpu->cycle += 2;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
    }
}
void ThumbLSREG(uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t B_bit = (inst >> 10) & 0x1;
    uint8_t Ro = (inst >> 6) & 0x7;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    cpu->DebugFunc = 18;
    if(L_bit){
        //LDR
        if(B_bit)cpu->Reg[Rd] = MemRead8(cpu->Reg[Ro] + cpu->Reg[Rb]);
        else{cpu->Reg[Rd] = MemRead32(cpu->Reg[Ro] + cpu->Reg[Rb]);}
        cpu->cycle += 2;
        if(Rd == PC){
            cpu->cycle += 2;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
    }
    else{
        //STR
        if(B_bit)MemWrite8(cpu->Reg[Ro] + cpu->Reg[Rb], cpu->Reg[Rd] & 0xff);
        else{MemWrite32(cpu->Reg[Ro] + cpu->Reg[Rb], cpu->Reg[Rd]);}
        cpu->cycle += 1;
    }
}
void ThumbLSBH(uint16_t inst){
    uint8_t H_bit = (inst >> 11) & 0x1;
    uint8_t S_bit = (inst >> 10) & 0x1;
    uint8_t Ro = (inst >> 6) & 0x7;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    cpu->DebugFunc = 20;
    if(H_bit == 0 && S_bit == 0){
        MemWrite16(cpu->Reg[Ro] + cpu->Reg[Rb], (uint16_t)(cpu->Reg[Rd] & 0xffff));
        cpu->cycle += 1;
    }
    else{
        //LDR
        if(H_bit){
            if(S_bit)cpu->Reg[Rd] = (uint32_t)((((int32_t)MemRead16(cpu->Reg[Ro] + cpu->Reg[Rb])) << 16) >> 16);
            else{cpu->Reg[Rd] = MemRead16(cpu->Reg[Ro] + cpu->Reg[Rb]);}
        }
        else{cpu->Reg[Rd] = (uint32_t)((((int32_t)MemRead8(cpu->Reg[Ro] + cpu->Reg[Rb])) << 24) >> 24);}
        cpu->cycle += 2;
        if(Rd == PC){
            cpu->cycle += 2;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
    }
}
void ThumbLSIMM(uint16_t inst){
    uint8_t B_bit = (inst >> 12) & 0x1;
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Offset = (inst >> 6) & 0x1f;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    cpu->DebugFunc = 19;
    if(L_bit){
        //LDR
        if(B_bit)cpu->Reg[Rd] = MemRead8(cpu->Reg[Rb] + Offset);
        else{cpu->Reg[Rd] = MemRead32(cpu->Reg[Rb] + (Offset << 2));}
        cpu->cycle += 2;
        if(Rd == PC){
            cpu->cycle += 2;
            cpu->fetchcache[1] = MemRead16(cpu->Reg[PC]);
            cpu->fetchcache[0] = MemRead16(cpu->Reg[PC] + 0x2);
            cpu->Reg[PC] += 0x2;
        }
    }
    else{
        if(B_bit)MemWrite8(cpu->Reg[Rb] + Offset, cpu->Reg[Rd]);
        else{
            MemWrite32(cpu->Reg[Rb] + (Offset << 2), cpu->Reg[Rd]);
        }
        cpu->cycle += 1;
    }
}
void ThumbAS(uint16_t inst){
    uint8_t I_bit = (inst >> 10) & 0x1;
    uint8_t Op = (inst >> 9) & 0x1;
    uint8_t Rn = (inst >> 6) & 0x7;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    uint32_t tmp = 0;
    cpu->DebugFunc = 13;
    tmp = cpu->Reg[Rs];
    //printf("Rn %d %08x, Rd %d %08x, Rs %d %08x\n", Rn, cpu->Reg[Rn], Rd, (cpu->Reg[Rs]) + (cpu->Reg[Rn]), Rs, cpu->Reg[Rs]);
    switch((Op << 1) | I_bit){
        case 0:
            cpu->Reg[Rd] = (cpu->Reg[Rs] + cpu->Reg[Rn]);
            CPSRUpdate(A_ADD, cpu->Reg[Rd], tmp, cpu->Reg[Rn]);
            break;
        case 1:
            cpu->Reg[Rd] = cpu->Reg[Rs] + Rn;
            CPSRUpdate(A_ADD, cpu->Reg[Rd], tmp, Rn);
            break;
        case 2:
            cpu->Reg[Rd] = cpu->Reg[Rs] - cpu->Reg[Rn];
            CPSRUpdate(A_SUB, cpu->Reg[Rd], tmp, cpu->Reg[Rn]);
            break;
        case 3:
            cpu->Reg[Rd] = cpu->Reg[Rs] - Rn;
            CPSRUpdate(A_SUB, cpu->Reg[Rd], tmp, Rn);
            break;
    }
    if(Rd == PC)cpu->cycle += 2;
}
void ThumbMVREG(uint16_t inst){
    uint8_t Op = (inst >> 11) & 0x3;
    uint8_t Offset = (inst >> 6) & 0x1f;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    cpu->DebugFunc = 12;
    switch(Op){
        case 0:
            //LSL
            if(Offset != 0)cpu->carry_out = (cpu->Reg[Rs] >> (32 - Offset)) & 0x1;
            cpu->Reg[Rd] = (cpu->Reg[Rs] << Offset);
            CPSRUpdate(LOG, cpu->Reg[Rd], cpu->Reg[Rs], Offset);
            break;
        case 1:
            //LSR
            if(Offset != 0)cpu->carry_out = (cpu->Reg[Rs] >> (Offset - 1)) & 0x1;
            cpu->Reg[Rd] = (cpu->Reg[Rs] >> Offset);
            CPSRUpdate(LOG, cpu->Reg[Rd], cpu->Reg[Rs], Offset);
            break;
        case 2:
            //ASR
            if(Offset != 0)cpu->carry_out = (cpu->Reg[Rs] >> (Offset - 1)) & 0x1;
            cpu->Reg[Rd] = (((int32_t)cpu->Reg[Rs]) >> Offset);
            CPSRUpdate(LOG, cpu->Reg[Rd], cpu->Reg[Rs], Offset);
            break;
    }
    if(Rd == PC)cpu->cycle += 2;
}
void ThumbIMM(uint16_t inst){
    uint8_t Op = (inst >> 11) & 0x3;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint8_t Offset = (inst) & 0xff;
    uint32_t tmp = 0;
    cpu->DebugFunc = 14;
    switch(Op){
        case 0:
        //mov
            tmp = cpu->Reg[Rd];
            cpu->Reg[Rd] = Offset;
            CPSRUpdate(MOVS, cpu->Reg[Rd], tmp, Offset);
            break;
        case 1:
            CPSRUpdate(A_SUB, cpu->Reg[Rd] - Offset, cpu->Reg[Rd], Offset);
            break;
        case 2:
            tmp = cpu->Reg[Rd];
            cpu->Reg[Rd] += Offset;
            CPSRUpdate(A_ADD, cpu->Reg[Rd], tmp, Offset);
            break;
        case 3:
            tmp = cpu->Reg[Rd];
            cpu->Reg[Rd] -= Offset;
            CPSRUpdate(A_SUB, cpu->Reg[Rd], tmp, Offset);
            break;
    }
    if(Rd == PC)cpu->cycle += 2;
}