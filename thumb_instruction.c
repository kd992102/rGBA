#include <stdint.h>
#include "arm7tdmi.h"
#include "thumb_instruction.h"

void ThumbMULLS(Gba_Cpu *cpu, uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Rb = (inst >> 8) & 0x7;
    uint8_t RegList = (inst) & 0xff;
    if(L_bit){
        for(int i=0;i<8;i++){
            if((RegList >> i) & 0x1){
                cpu->Reg[i] = MemRead32(cpu, cpu->Reg[Rb]);
                cpu->Reg[Rb] = cpu->Reg[Rb] + 4;
            }
        }
    }
    else{
        for(int i=0;i<8;i++){
            if((RegList >> i) & 0x1){
                MemWrite32(cpu, cpu->Reg[Rb], cpu->Reg[i]);
                cpu->Reg[Rb] = cpu->Reg[Rb] + 4;
            }
        }
    }
}
void ThumbCondB(Gba_Cpu *cpu, uint16_t inst){
    uint8_t cond = (inst >> 8) & 0xf;
    int8_t Offset = inst & 0xff;
    cpu->Cond = cond;
    //Offset = Offset << 1;
    printf("Offset:%08x, PC:%08x\n", Offset, cpu->Reg[PC]);
    if(((Offset >> 7) & 0x1) == 0x1){
        cpu->fetchcache[1] = MemRead16(cpu, cpu->Reg[PC] + Offset);
        cpu->fetchcache[0] = MemRead16(cpu, cpu->Reg[PC] + Offset + 0x2);
        cpu->Ptr = cpu->Reg[PC];
        cpu->Ptr += 0x2;
    }
    else{
        cpu->fetchcache[1] = MemRead16(cpu, cpu->Reg[PC] + Offset);
        cpu->fetchcache[0] = MemRead16(cpu, cpu->Reg[PC] + Offset + 0x2);
        cpu->Ptr = cpu->Reg[PC];
        cpu->Ptr += 0x2;
    }
        //printf("Ptr:0x%08x\n", cpu->Ptr);
}
void ThumbSWI(Gba_Cpu *cpu, uint16_t inst){
    uint8_t value = inst & 0xff;
    cpu->Reg[LR] = cpu->Ptr + 0x2;
    cpu->Reg[PC] = 0x00000008;
    cpu->CpuMode = ARM_MODE;
}
void ThumbUCOND(Gba_Cpu *cpu, uint16_t inst){
    uint16_t Offset = inst & 0x7ff;
    Offset = Offset << 1;
    if((Offset >> 11)){
        cpu->fetchcache[1] = MemRead16(cpu, cpu->Reg[PC] - Offset) & 0xffff;
        cpu->fetchcache[0] = MemRead16(cpu, cpu->Reg[PC] - Offset + 0x2) & 0xffff;
        cpu->fetchcache[2] = 0x0;
        cpu->Ptr = cpu->Reg[PC] - Offset - 0x2;
    }
    else{
        cpu->fetchcache[1] = MemRead16(cpu, cpu->Reg[PC] + Offset) & 0xffff;
        cpu->fetchcache[0] = MemRead16(cpu, cpu->Reg[PC] + Offset + 0x2) & 0xffff;
        cpu->fetchcache[2] = 0x0;
        cpu->Ptr = cpu->Reg[PC] + Offset - 0x2;
    }

}
void ThumbLONGBL(Gba_Cpu *cpu, uint16_t inst1, uint16_t inst2){
    uint8_t H1_bit = (inst1 >> 11) & 0x1;
    uint8_t Offset1 = (inst1) & 0x7ff;
    uint8_t H2_bit = (inst2 >> 11) & 0x1;
    uint8_t Offset2 = (inst2) & 0x7ff;
    uint32_t tmp = 0;
    if(!H1_bit){
        cpu->Reg[LR] = cpu->Reg[PC] + (Offset1 << 12);
        tmp = cpu->Ptr + 0x2;
        cpu->Reg[PC] = cpu->Reg[LR] + (Offset2 << 1);
        cpu->Reg[LR] = tmp | 0x1;
    }
    else{
        cpu->Reg[LR] = cpu->Reg[PC] + (Offset2 << 12);
        tmp = cpu->Ptr + 0x2;
        cpu->Reg[PC] = cpu->Reg[LR] + (Offset1 << 1);
        cpu->Reg[LR] = tmp | 0x1;
    }
}
void ThumbLSH(Gba_Cpu *cpu, uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Offset = (inst >> 6) & 0x1f;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    if(L_bit){
        //LDR
        cpu->Reg[Rd] = MemRead16(cpu, cpu->Reg[Rb] + (Offset << 1)) & 0xffff;
    }
    else{
        MemWrite16(cpu, cpu->Reg[Rb] + (Offset << 1), cpu->Reg[Rd] & 0xffff);
    }
}
void ThumbSPLS(Gba_Cpu *cpu, uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Rd = (inst >> 8) & 0x1f;
    uint16_t Word = (inst) & 0xff;
    if(L_bit){
        //LDR
        cpu->Reg[Rd] = MemRead32(cpu, cpu->Reg[SP] + (Word << 2));
    }
    else{
        MemWrite32(cpu, cpu->Reg[SP] + (Word << 2), cpu->Reg[Rd]);
    }
}
void ThumbLADDR(Gba_Cpu *cpu, uint16_t inst){
    uint8_t SP_bit = (inst >> 11) & 0x1;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint16_t Word = (inst) & 0xff;
    if(SP_bit){
        //
        cpu->Reg[Rd] = cpu->Reg[SP] + (Word << 2);
    }
    else{
        //PC bit 1 force to 0
        cpu->Reg[Rd] = cpu->Reg[PC] & 0xfffffffd + (Word << 2);
    }
}
void ThumbADDSP(Gba_Cpu *cpu, uint16_t inst){
    uint8_t S_bit = (inst >> 7) & 0x1;
    uint8_t Word = (inst) & 0x7f;
    if(S_bit){
        //
        cpu->Reg[SP] = cpu->Reg[SP] + (Word << 1);
    }
    else{
        //PC bit 1 force to 0
        cpu->Reg[SP] = cpu->Reg[SP] - (Word << 1);
    }
}
void ThumbPPREG(Gba_Cpu *cpu, uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t R_bit = (inst >> 8) & 0x1;
    uint8_t RegList = (inst) & 0xff;
    if(L_bit){
        //POP
        if(R_bit){
            cpu->Reg[PC] = MemRead32(cpu, cpu->Reg[SP]);
            cpu->Reg[SP] = cpu->Reg[SP] + 4;
        }
        for(int i=0;i<8;i++){
            if((RegList >> i) & 0x1){
                cpu->Reg[i] = MemRead32(cpu, cpu->Reg[SP]);
                cpu->Reg[SP] = cpu->Reg[SP] + 4;
            }
        }
    }
    else{
        //PUSH
        if(R_bit){
                cpu->Reg[SP] = cpu->Reg[SP] - 4;
                MemWrite32(cpu, cpu->Reg[SP], cpu->Reg[LR]);
        }
        for(int i=7;i>=0;i--){
            if((RegList >> i) & 0x1){
                cpu->Reg[SP] = cpu->Reg[SP] - 4;
                MemWrite32(cpu, cpu->Reg[SP], cpu->Reg[i]);
            }
        }
    }
}
void ThumbALU(Gba_Cpu *cpu, uint16_t inst){
    uint8_t Op = (inst >> 6) & 0xf;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    switch(Op){
        case 0:
            //AND
            cpu->Reg[Rd] = (cpu->Reg[Rd] & cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 1:
            //EOR
            cpu->Reg[Rd] = (cpu->Reg[Rd] ^ cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 2:
            //LOL
            cpu->Reg[Rd] = (cpu->Reg[Rd] << cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 3:
            //LOR
            cpu->Reg[Rd] = (cpu->Reg[Rd] >> cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 4:
            //AOR
            cpu->Reg[Rd] = ((int16_t)(cpu->Reg[Rd]) >> (cpu->Reg[Rs]));
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 5:
            //ADC
            cpu->Reg[Rd] = (cpu->Reg[Rd]) + (cpu->Reg[Rs]) + ((cpu->CPSR >> 29) & 0x1);
            CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rd] + ((cpu->CPSR >> 29) & 0x1), cpu->Reg[Rs]);
            break;
        case 6:
            //SBC
            cpu->Reg[Rd] = (cpu->Reg[Rd]) - (cpu->Reg[Rs]) - !((cpu->CPSR >> 29) & 0x1);
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], cpu->Reg[Rd] - !((cpu->CPSR >> 29) & 0x1), cpu->Reg[Rs]);
            break;
        case 7:
            //ROR
            cpu->Reg[Rd] = (cpu->Reg[Rd]) << (32 - cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rd], cpu->Reg[Rs]);
            break;
        case 8:
            //TST
            CPSRUpdate(cpu, LOG, (cpu->Reg[Rd] & cpu->Reg[Rs]), (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 9:
            //NEG
            cpu->Reg[Rd] = (0 - (cpu->Reg[Rs]));
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 10:
            //CMP
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd] - cpu->Reg[Rs], cpu->Reg[Rd], cpu->Reg[Rs]);
            break;
        case 11:
            //CMN
            CPSRUpdate(cpu, A_ADD, (cpu->Reg[Rd]) + (cpu->Reg[Rs]), (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 12:
            //ORR
            cpu->Reg[Rd] = (cpu->Reg[Rd] | cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 13:
            //MUL
            cpu->Reg[Rd] = (cpu->Reg[Rd]) * (cpu->Reg[Rs]);
            CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 14:
            //BIC
            cpu->Reg[Rd] = (cpu->Reg[Rd]) & ~(cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
        case 15:
            //MVN
            cpu->Reg[Rd] = ~(cpu->Reg[Rs]);
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], (cpu->Reg[Rd]), (cpu->Reg[Rs]));
            break;
    }
}
void ThumbBX(Gba_Cpu *cpu, uint16_t inst){
    uint8_t Op = (inst >> 8) & 0x3;
    uint8_t H1 = (inst >> 7) & 0x1;
    uint8_t H2 = (inst >> 6) & 0x1;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    if(H1)Rs += 8;
    if(H2)Rd += 8;
    switch(Op){
        case 0:
            cpu->Reg[Rd] += cpu->Reg[Rs];
            break;
        case 1:
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd] - cpu->Reg[Rs], cpu->Reg[Rd], cpu->Reg[Rs]);
            break;
        case 2:
            cpu->Reg[Rd] = cpu->Reg[Rs];
            break;
        case 3:
            cpu->Reg[PC] = cpu->Reg[Rs];
            if(cpu->Reg[PC] & 0x1)cpu->dMode = THUMB_MODE;
            else{cpu->dMode = ARM_MODE;}
            break;
    }
}
void ThumbPCLOAD(Gba_Cpu *cpu, uint16_t inst){
    uint8_t Rd = (inst >> 8) & 0x7;
    uint8_t Word = inst & 0xff;
    cpu->Reg[Rd] = MemRead32(cpu, cpu->Reg[PC] - 0x2 + (Word << 2));
}
void ThumbLSREG(Gba_Cpu *cpu, uint16_t inst){
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t B_bit = (inst >> 10) & 0x1;
    uint8_t Ro = (inst >> 6) & 0x7;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    if(L_bit){
        //LDR
        if(B_bit)cpu->Reg[Rd] = MemRead8(cpu, cpu->Reg[Ro] + cpu->Reg[Rb]);
        else{cpu->Reg[Rd] = MemRead32(cpu, cpu->Reg[Ro] + cpu->Reg[Rb]);}
    }
    else{
        //STR
        if(B_bit)MemWrite8(cpu, cpu->Reg[Ro] + cpu->Reg[Rb], cpu->Reg[Rd] & 0xff);
        else{MemWrite32(cpu, cpu->Reg[Ro] + cpu->Reg[Rb], cpu->Reg[Rd]);}
    }
}
void ThumbLSBH(Gba_Cpu *cpu, uint16_t inst){
    uint8_t H_bit = (inst >> 11) & 0x1;
    uint8_t S_bit = (inst >> 10) & 0x1;
    uint8_t Ro = (inst >> 6) & 0x7;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    if(!(H_bit | S_bit)){
        MemWrite16(cpu, cpu->Reg[Ro] + cpu->Reg[Rb], cpu->Reg[Rd] & 0xffff);
    }
    else{
        //LDR
        if(H_bit){
            if(S_bit)cpu->Reg[Rd] = (uint32_t)((((int32_t)MemRead16(cpu, cpu->Reg[Ro] + cpu->Reg[Rb]) & 0xffff) << 16) >> 16);
            else{cpu->Reg[Rd] = MemRead16(cpu, cpu->Reg[Ro] + cpu->Reg[Rb]) & 0xffff;}
        }
        else{cpu->Reg[Rd] = (uint32_t)((((int32_t)MemRead8(cpu, cpu->Reg[Ro] + cpu->Reg[Rb]) & 0xff) << 24) >> 24);}
    }
}
void ThumbLSIMM(Gba_Cpu *cpu, uint16_t inst){
    uint8_t B_bit = (inst >> 12) & 0x1;
    uint8_t L_bit = (inst >> 11) & 0x1;
    uint8_t Offset = (inst >> 6) & 0x1f;
    uint8_t Rb = (inst >> 3) & 0x7;
    uint8_t Rd = (inst) & 0x7;
    printf("Rb:%d, Rb:%08x, OFfset:%08x\n", Rb, cpu->Reg[Rb], Offset);
    if(L_bit){
        //LDR
        if(B_bit)cpu->Reg[Rd] = MemRead8(cpu, cpu->Reg[Rb] + Offset) & 0xff;
        else{cpu->Reg[Rd] = MemRead32(cpu, cpu->Reg[Rb] + (Offset << 2));}
    }
    else{
        if(B_bit)MemWrite8(cpu, cpu->Reg[Rb] + Offset, cpu->Reg[Rd] & 0xff);
        else{MemWrite32(cpu, cpu->Reg[Rb] + (Offset << 2), cpu->Reg[Rd]);}
    }
}
void ThumbAS(Gba_Cpu *cpu, uint16_t inst){
    uint8_t I_bit = (inst >> 10) & 0x1;
    uint8_t Op = (inst >> 9) & 0x1;
    uint8_t Rn = (inst >> 6) & 0x7;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    //printf("Rn %d %08x, Rd %d %08x, Rs %d %08x\n", Rn, cpu->Reg[Rn], Rd, (cpu->Reg[Rs]) + (cpu->Reg[Rn]), Rs, cpu->Reg[Rs]);
    switch((Op << 1) | I_bit){
        case 0:
            cpu->Reg[Rd] = (cpu->Reg[Rs] + cpu->Reg[Rn]);
            CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rs], cpu->Reg[Rn]);
            break;
        case 1:
            cpu->Reg[Rd] = cpu->Reg[Rs] + Rn;
            CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], cpu->Reg[Rs], Rn);
            break;
        case 2:
            cpu->Reg[Rd] = cpu->Reg[Rs] - cpu->Reg[Rn];
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], cpu->Reg[Rs], cpu->Reg[Rn]);
            break;
        case 3:
            cpu->Reg[Rd] = cpu->Reg[Rs] - Rn;
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], cpu->Reg[Rs], Rn);
            break;
    }
}
void ThumbMVREG(Gba_Cpu *cpu, uint16_t inst){
    uint8_t Op = (inst >> 11) & 0x3;
    uint8_t Offset = (inst >> 6) & 0x1f;
    uint8_t Rs = (inst >> 3) & 0x7;
    uint8_t Rd = inst & 0x7;
    switch(Op){
        case 0:
            //LSL
            cpu->Reg[Rd] = (cpu->Reg[Rs] << Offset);
            if(Offset != 0)cpu->carry_out = (cpu->Reg[Rs] >> (32 - Offset)) & 0x1;
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rs], Offset);
            break;
        case 1:
            //LSR
            cpu->Reg[Rd] = (cpu->Reg[Rs] >> Offset);
            if(Offset != 0)cpu->carry_out = (cpu->Reg[Rs] >> (Offset - 1)) & 0x1;
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rs], Offset);
            break;
        case 2:
            //ASR
            cpu->Reg[Rd] = (((int16_t)cpu->Reg[Rs]) >> Offset);
            if(Offset != 0)cpu->carry_out = (cpu->Reg[Rs] >> (Offset - 1)) & 0x1;
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], cpu->Reg[Rs], Offset);
            break;
    }
}
void ThumbIMM(Gba_Cpu *cpu, uint16_t inst){
    uint8_t Op = (inst >> 11) & 0x3;
    uint8_t Rd = (inst >> 8) & 0x7;
    uint8_t Offset = (inst) & 0xff;
    uint32_t tmp = 0;
    switch(Op){
        case 0:
            tmp = cpu->Reg[Rd];
            cpu->Reg[Rd] = Offset;
            CPSRUpdate(cpu, LOG, cpu->Reg[Rd], tmp, Offset);
            break;
        case 1:
            //cpu->Reg[Rd] = Offset;
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd] - Offset, cpu->Reg[Rd], Offset);
            break;
        case 2:
            tmp = cpu->Reg[Rd];
            cpu->Reg[Rd] += Offset;
            CPSRUpdate(cpu, A_ADD, cpu->Reg[Rd], tmp, Offset);
            break;
        case 3:
            tmp = cpu->Reg[Rd];
            cpu->Reg[Rd] -= Offset;
            CPSRUpdate(cpu, A_SUB, cpu->Reg[Rd], tmp, Offset);
            break;
    }
}