u8 IsBranch = 0;
extern struct arm7tdmi cpu;

int chk_cond(u8 cond){
    u8 N = ((cpu.CPSR >> 31) & 0x1);
    u8 Z = ((cpu.CPSR >> 30) & 0x1);
    u8 C = ((cpu.CPSR >> 29) & 0x1);
    u8 V = ((cpu.CPSR >> 28) & 0x1);
    switch(cond){
        case 0:
            if(Z == 1)return 1;
            return 0;
        case 1:
            if(Z == 0)return 1;
            return 0;
        case 2:
            if(C == 1)return 1;
            return 0;
        case 3:
            if(C == 0)return 1;
            return 0;
        case 4:
            if(N == 1)return 1;
            return 0;
        case 5:
            if(N == 0)return 1;
            return 0;
        case 6:
            if(V == 1)return 1;
            return 0;
        case 7:
            if(V == 0)return 1;
            return 0;
        case 8:
            if(C == 1 && N == 0)return 1;
            return 0;
        case 9:
            if(C == 0 || Z == 1)return 1;
            return 0;
        case 10:
            if(N == V)return 1;
            return 0;
        case 11:
            if(N != V)return 1;
            return 0;
        case 12:
            if((Z == 0) & (N == V))return 1;
            return 0;
        case 13:
            if((Z == 1) | (N != V))return 1;
            return 0;
        case 14:
            return 1;
        default:
            return 0;
    }
}
/*
//read 8 bytes and decode it
void cpu_prefetch(struct arm7tdmi cpu, u32 *gbarom_buf){
    if(IsBranch){
        cpu.cache1 = 0;
        cpu.cache2 = 0;
    }
    cpu.cache2 = *((u32 *)((u8 *)gbarom_buf + cpu.reg[15] - 8));
    
    dec_inst(cpu.cache2);
    cpu.cache1 = cpu.cache2;
    cpu.cache2 = *((u32 *)((u8 *)gbarom_buf + cpu.reg[15] - 4));
    //printf("cache1 : 0x%08x, cache2 : 0x%08x, pc : %d\n", cpu.cache1, cpu.cache2, cpu.pc);
    dec_inst(cpu.cache2);
    //exe_inst(cpu.cache1);
}*/
/*
struct Mem_Map{
    unsigned int bios[0x1000];
    unsigned int space1[0x7FF000];
    unsigned int OnBoardWRAM[0x10000]; //for interrupt
    unsigned int space2[0x3F0000];
    unsigned int OnChipWRAM[0x2000];
    unsigned int space3[0x3FE000];
    unsigned int IORegisters[0x100];
    unsigned int space4[0x3FFF00];
};*/