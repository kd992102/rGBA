#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include "typedef.h"
#include "arm7tdmi.h"
#include "arm_instruction.h"
#include "thumb_instruction.h"
#include "cpu.h"
//#include "cart.h"


Gba_Cpu *cpu;
//struct gba_rom *gba_rom;
FILE *bios;
FILE *rom;
uint32_t bios_size;
uint32_t rom_size;
static const char bios_file_name[] = "gba_bios.bin";
static const char rom_file_name[] = "pok_g_386.gba";

void TestRegInit();
void ArmSetTest();

int main(int argc, char *argv[]){

    bios = fopen(bios_file_name, "rb");
    cpu = malloc(sizeof(Gba_Cpu));
    //printf("Gba_Cpu:0x%08x\n", cpu);
    cpu->CpuMode = SYSTEM;
    cpu->dMode = ARM_MODE;
    cpu->GbaMem = malloc(sizeof(Gba_Memory));
    Init_GbaMem(cpu->GbaMem);

    fseek(bios, 0, SEEK_END);
    bios_size = ftell(bios);
    printf("Bios File size : %u bytes\n", bios_size);
    fseek(bios, 0, SEEK_SET);
    
    if(fread(cpu->GbaMem->GIMem->BIOS, sizeof(uint8_t), bios_size, bios)){
        puts("Bios read success");
        fclose(bios);
        bios = NULL;
    }
    else{
        puts("Bios read failed");
        fclose(bios);
        bios = NULL;
        return 1;
    }

    /*rom = fopen(rom_file_name, "rb");
    fseek(rom, 0, SEEK_END);
    rom_size = ftell(rom);
    printf("Rom File size : %u bytes\n", rom_size);
    fseek(rom, 0, SEEK_SET);

    if(fread(cpu->GbaMem->ExMem->Game_ROM1, sizeof(uint8_t), rom_size, rom)){
        puts("ROM read success");
        fclose(rom);
        rom = NULL;
    }
    else{
        puts("ROM read failed");
        fclose(rom);
        rom = NULL;
        return 1;
    }*/
    
    cpu->Ptr = 0x00000000;//ROM File ptr
    InitCpu(cpu, cpu->Ptr);
    //Program Counter = fetch instruction
    cpu->cycle = 0;
    uint8_t Cmode = 0x1F;
    for(;;){
        //printf("--------Current Instruction--------\n");
        //printf("[fetch]:%08x, instruction:%08x\n", cpu->Ptr, cpu->fetchcache[0]);
        //printf("[decode]:%08x, instruction:%08x\n", cpu->Ptr - cpu->InstOffset, cpu->fetchcache[1]);
        //printf("[execute]:%08x, instruction:%08x\n", cpu->Ptr - (cpu->InstOffset * 2), cpu->fetchcache[2]);
        //Execute
        //printf("LR:%08x\n", cpu->Reg[LR]);
        Cmode = ChkCPUMode(cpu);
        //printf("Cmode:%x\n", cpu->CpuMode);
        CpuExecute(cpu, cpu->fetchcache[2]);
        //printf("[--execute--]\n");
        if(cpu->dMode == THUMB_MODE)cpu->InstOffset = 0x2;
        else{cpu->InstOffset = 0x4;}

        cpu->Ptr += cpu->InstOffset;//0x4
        RecoverReg(cpu, Cmode);
        if(cpu->cycle == 858){
            printf("Cycle:%d\n", cpu->cycle);
            CpuStatus(cpu);
            //printf("MemADDR:%08x\n", MemRead32(cpu, 0x27C));
            //if(cpu->cycle == 852)break;
            break;
        }

        cpu->Reg[PC] = cpu->Ptr;
        PreFetch(cpu, cpu->Ptr);//fetch new instruction
        
        //getchar();
    }


    Release_GbaMem(cpu->GbaMem);
    free(cpu);
    cpu = NULL;
    return 0;
}
void TestRegInit(){
    cpu->dMode = ARM_MODE;
    cpu->CPSR = 0x13;
    cpu->Reg[R0] = 0x4000;
    cpu->Reg[R1] = 0x1000;
    cpu->Reg[R2] = 0x2;
}
void ArmSetTest(){
    uint32_t art[9] = {0xE1A00101,/*MOV R0,R1,LSL #2*/ 
                      0xE1A00211,/*MOV R0,R1,LSL R2*/ 
                      0xE1A00121,/*MOV R0,R1,LSR #2*/ 
                      0xE1A00231,/*MOV R0,R1,LSR R2*/ 
                      0xE1A00141,/*MOV R0,R1,ASR #2*/ 
                      0xE1A00251,/*MOV R0,R1,ASR R2*/ 
                      0xE1A00161,/*MOV R0,R1,ROR #2*/ 
                      0xE1A00271,/*MOV R0,R1,ROR R2*/ 
                      0xE1A00061/*MOV R0,R1,RRX*/};
    uint32_t alu[13] = {0xE2900001,/*adds r0,r0,#1*/ 
                      0xE2000902,/*and  r0,r0,#0x8000*/ 
                      0xE3C00902,/*bic  r0,r0,#0x8000*/ 
                      0xE3700001,/*cmn  r0,#1*/
                      0xE3500001,/*cmp  r0,#1*/
                      0xE2200902,/*eor r0,r0,#0x8000*/
                      0xE3A00001,/*MOV R0, #1*/ 
                      0xE3E00001,/*MVN R0, #1*/ 
                      0xE3800902,/*ORR R0, R0, #0x8000*/ 
                      0xE2600000,/*RSB R0, R0, #0*/
                      0xE2500001,/*SUBS R0, R0, #1*/
                      0xE3300902,/*TEQ R0, #0x8000*/
                      0xE3100902/*TST R0, #0x8000*/};
    uint32_t blx[3] = {0xEA000018,/*b  0x68*/ 
                      0xEB00004C,/*bl 0x138*/ 
                      0xE12FFF10};/*bx r0*/
    uint32_t ldr[7] = {0xE891001D,/*ldmia r1, {r0, r2-r4}*/
                      0xE911001D,/*ldmdb r1, {r0, r2-r4}*/
                      0xE5910000,/*ldr r0,[r1]*/
                      0xE5D10000,/*ldrb r0,[r1]*/
                      0xE1D100B0,/*ldrh r0,[r1]*/
                      0xE1D100D0,/*ldrsb r0,[r1]*/
                      0xE1D100F0/*ldrsh r0,[r1]*/};
    uint32_t str[5] = {0xE881001D,/*stmia r1, {r0, r2-r4}*/
                      0xE921001D,/*stmdb r1!, {r0, r2-r4}*/
                      0xE5810004,/*str r0,[r1,#4]*/
                      0xE5E10004,/*STRB R0, [R1, #4]!*/
                      0xE14100B2/*STRH R0, [R1, #-2]*/};
    uint32_t mul[6] = {0xE0200291,/*MLA R0, R1, R2, R0*/
                      0xE0000291,/*MUL R0, R1, R2*/
                      0xE0E10092,/*SMLAL R0, R1, R2, R3*/
                      0xE0C10092,/*SMULL R0, R1, R2, R3*/
                      0xE0A10092,/*UMLAL R0, R1, R2, R3*/
                      0xE0810092/*UMULL R0, R1, R2, R3*/};
    uint32_t mrs[2] = {0xE10F0000,/*MRS R0, CPSR*/
                      0xE129F000/*MSR CPSR, R0*/};
    uint32_t swi[2] = {0xE10F0000,/*MRS R0, CPSR*/
                      0xE129F000/*MSR CPSR, R0*/};
    uint32_t swp[2] = {0xE1010090,/*SWP R0, R0, [R1] */
                      0xE1410090/*SWPB R0, R0, [R1] */};
    int i = 0;
    for(i=0;i<9;i++){
        cpu->fetchcache[2] = art[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        if(i == 0 && cpu->Reg[R0] != 0x4000)printf("LSL Imm error\n");
        if(i == 1 && cpu->Reg[R0] != 0x4000)printf("LSL Reg error\n");
        if(i == 2 && cpu->Reg[R0] != 0x400)printf("LSR Imm error\n");
        if(i == 3 && cpu->Reg[R0] != 0x400)printf("LSR Reg error\n");
        if(i == 4 && cpu->Reg[R0] != 0x400)printf("ASR Imm error\n");
        if(i == 5 && cpu->Reg[R0] != 0x400)printf("ASR Reg error\n");
        if(i == 6 && cpu->Reg[R0] != 0x400)printf("ROR Imm error\n");
        if(i == 7 && cpu->Reg[R0] != 0x400)printf("ROR Reg error\n");
        if(i == 8 && cpu->Reg[R0] != 0x800)printf("RRX error\n");
        //else{}
    }
    printf("-->[ Barrel Shifter ] check done.\n");
    TestRegInit();
    for(i=0;i<13;i++){
        cpu->fetchcache[2] = alu[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        if(i == 0){if(cpu->Reg[R0] != 0x4001)printf("ADDS error\n");}
        if(i == 1){if(cpu->Reg[R0] != 0x0)printf("AND error\n");}
        if(i == 2){if(cpu->Reg[R0] != 0x4000)printf("BIC error\n");}
        if(i == 3){if(cpu->Reg[R0] != 0x4000 || (cpu->CPSR & 0xf0000000) != 0x0)printf("CMN error\n");}
        if(i == 4){if(cpu->Reg[R0] != 0x4000 || (cpu->CPSR & 0xf0000000) != 0x20000000)printf("CMP error\n");}
        if(i == 5){if(cpu->Reg[R0] != 0xc000)printf("EOR error\n");}
        if(i == 6){if(cpu->Reg[R0] != 0x1)printf("MOV error\n");}
        if(i == 7){if(cpu->Reg[R0] != 0xfffffffe)printf("MVN error\n");}
        if(i == 8){if(cpu->Reg[R0] != 0xc000)printf("ORR error\n");}
        if(i == 9){if(cpu->Reg[R0] != 0xffffc000)printf("RSB error\n");}
        if(i == 10){if(cpu->Reg[R0] != 0x3fff || (cpu->CPSR & 0xf0000000) != 0x20000000)printf("SUBS error\n");}
        if(i == 11){if(cpu->Reg[R0] != 0x4000 || (cpu->CPSR & 0xf0000000) != 0x0)printf("TEQ error\n");}
        if(i == 12){if(cpu->Reg[R0] != 0x4000 || (cpu->CPSR & 0xf0000000) != 0x40000000)printf("TST error\n");}
        //else{}
        TestRegInit();
    }
    printf("-->[ ALU ] check done.\n");
    
    for(i=0;i<3;i++){
        cpu->fetchcache[2] = blx[i];
        //printf("fetch:%08x\tdecode:%08x\texecute:%08x\n", cpu->fetchcache[0], cpu->fetchcache[1], cpu->fetchcache[2]);
        CpuExecute(cpu, cpu->fetchcache[2]);
        cpu->Ptr += cpu->InstOffset;
        PreFetch(cpu, cpu->Ptr);
        if(i == 0){
            if(cpu->Reg[PC] != 0x6C){
                printf("PC:%08x, LR:%08x\n", cpu->Reg[PC], cpu->Reg[LR]);
                printf("BL error\n");
                printf("B error\n");
            }
        }
        if(i == 1){
            if(cpu->Reg[PC] != 0x13C || cpu->Reg[LR] != 0x8){
                printf("PC:%08x, LR:%08x\n", cpu->Reg[PC], cpu->Reg[LR]);
                printf("BL error\n");
            }
        }
        if(i == 2){if(cpu->dMode != THUMB_MODE)printf("BX error\n");}
        //else{}
        TestRegInit();
        cpu->Reg[R0] = 0x1;
        cpu->Reg[PC] = 0x8;
    }
    printf("-->[ Branch ] check done.\n");

    TestRegInit();
    //printf("i:%d\n", i);
    for(i=0;i<7;i++){
        cpu->fetchcache[2] = ldr[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        //printf("i:%d\n", i);
        if(i == 0){
            //printf("R0:%08x\n", cpu->Reg[R0]);
            if(cpu->Reg[R0] != MemRead32(cpu, cpu->Reg[R1]) || cpu->Reg[R4] != MemRead32(cpu, cpu->Reg[R1] + 0xC))printf("LDMIA error\n");
        }
        if(i == 1){
            //printf("R0:%08x, R1-0x10:%08x, R4:%08x, R1+0x4:%08x\n", cpu->Reg[R0], MemRead32(cpu, cpu->Reg[R1] - 0x10), cpu->Reg[R4], MemRead32(cpu, cpu->Reg[R1] - 0x4));
            if(cpu->Reg[R0] != MemRead32(cpu, cpu->Reg[R1] - 0x10) || cpu->Reg[R4] != MemRead32(cpu, cpu->Reg[R1] - 0x4))printf("LDMDB error\n");
        }
        if(i == 2){
            //printf("R0:%08x\n", cpu->Reg[R0]);
            if(cpu->Reg[R0] != MemRead32(cpu, cpu->Reg[R1]))printf("LDR error\n");
        }
        if(i == 3){
            //printf("R0:%08x\n", cpu->Reg[R0]);
            //printf("data:%08x\n", MemRead8(cpu, cpu->Reg[R1]));
            if(cpu->Reg[R0] != MemRead8(cpu, cpu->Reg[R1]))printf("LDRB error\n");
        }
        if(i == 4){
            //printf("R0:%08x\n", cpu->Reg[R0]);
            if(cpu->Reg[R0] != MemRead16(cpu, cpu->Reg[R1]))printf("LDRH error\n");
        }
        if(i == 5){
            if((MemRead8(cpu, cpu->Reg[R1]) >> 7)){
                //printf("R0:%08x, data:%08x\n", cpu->Reg[R0], MemRead8(cpu, cpu->Reg[R1]));
                if(cpu->Reg[R0] != (MemRead8(cpu, cpu->Reg[R1]) | 0xffffff00))printf("LDRSB error\n");
            }
            else{
                //printf("R0:%08x, data:%08x\n", cpu->Reg[R0], MemRead8(cpu, cpu->Reg[R1]));
                if(cpu->Reg[R0] != MemRead8(cpu, cpu->Reg[R1]))printf("LDRSB error\n");
            }
        }
        if(i == 6){
            if((MemRead16(cpu, cpu->Reg[R1]) >> 15)){
                //printf("R0:%08x\n", cpu->Reg[R0]);
                if(cpu->Reg[R0] != (MemRead16(cpu, cpu->Reg[R1]) | 0xffff0000))printf("LDRSH error\n");
            }
            else{
                //printf("R0:%08x\n", cpu->Reg[R0]);
                if(cpu->Reg[R0] != MemRead16(cpu, cpu->Reg[R1]))printf("LDRSH error\n");
            }
        }
        cpu->Reg[R1] = 0x1C;
    }
    printf("-->[ Load ] check done.\n");

    TestRegInit();
    for(i=0;i<4;i++){
        cpu->fetchcache[2] = str[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        if(i == 0){
            if(cpu->Reg[R0] != MemRead32(cpu, cpu->Reg[R1]) || cpu->Reg[R4] != MemRead32(cpu, cpu->Reg[R1] + 0xC))printf("STMIA error\n");
        }
        if(i == 1){
            //printf("R0:%08x, R1:%08x, data:%08x\n", cpu->Reg[R0], cpu->Reg[R1], MemRead32(cpu, cpu->Reg[R1] - 0x8));
            if(cpu->Reg[R0] != MemRead32(cpu, cpu->Reg[R1]) || cpu->Reg[R4] != MemRead32(cpu, cpu->Reg[R1] + 0xC))printf("STMDB error\n");
        }
        if(i == 2){
            if(cpu->Reg[R0] != MemRead32(cpu, cpu->Reg[R1] + 0x4))printf("STR error\n");
        }
        if(i == 3){
            if((cpu->Reg[R0] & 0xff) != MemRead8(cpu, cpu->Reg[R1] + 0x4))printf("STRB error\n");
        }
        if(i == 4){
            if((cpu->Reg[R0] & 0xffff) != MemRead16(cpu, cpu->Reg[R1] - 0x2))printf("STRH error\n");
        }
    }
    printf("-->[ Store ] check done.\n");

    TestRegInit();
    for(i=0;i<5;i++){
        cpu->fetchcache[2] = mul[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        if(i == 0){
            if(cpu->Reg[R0] != 0x6000)printf("MLA error\n");
        }
        if(i == 1){
            if(cpu->Reg[R0] != 0x2000)printf("MUL error\n");
        }
        if(i == 2){
            //printf("R0:%08x\n", cpu->Reg[R0]);
            if(cpu->Reg[R0] != 0xC000)printf("SMLAL error\n");
        }
        if(i == 3){
            if(cpu->Reg[R0] != 0x8000 || cpu->Reg[R1] != 0x0)printf("SMULL error\n");
        }
        if(i == 4){
            if(cpu->Reg[R0] != 0xC000)printf("UMLAL error\n");
        }
        if(i == 5){
            if(cpu->Reg[R0] != 0x8000 || cpu->Reg[R1] != 0x0)printf("UMULL error\n");
        }
        TestRegInit();
    }
    printf("-->[ Multiple ] check done.\n");

    TestRegInit();
    for(i=0;i<2;i++){
        cpu->fetchcache[2] = mrs[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        if(i == 0){
            if(cpu->Reg[R0] != cpu->CPSR)printf("MRS error\n");
        }
        if(i == 1){
            if(cpu->Reg[R0] != cpu->CPSR)printf("MSR error\n");
        }
        TestRegInit();
    }
    printf("-->[ PSRT ] check done.\n");

    uint32_t old_reg;
    uint32_t old_mem;

    TestRegInit();
    for(i=0;i<2;i++){
        old_reg = cpu->Reg[R0];
        old_mem = MemRead32(cpu, cpu->Reg[R1]);
        cpu->fetchcache[2] = swp[i];
        CpuExecute(cpu, cpu->fetchcache[2]);
        if(i == 0){
            printf("%08x, %08x, %08x, %08x\n", cpu->Reg[R0], old_mem, old_reg, MemRead32(cpu, cpu->Reg[R1]));
            if(cpu->Reg[R0] != old_mem || old_reg != MemRead32(cpu, cpu->Reg[R1]))printf("SWP error\n");
        }
        if(i == 1){
            if(cpu->Reg[R0] != (old_mem & 0xff) || ((old_reg & 0xff) + (old_mem & 0xffffff00)) != MemRead32(cpu, cpu->Reg[R1]))printf("SWPB error\n");
        }
        TestRegInit();
    }
    printf("-->[ SWP ] check done.\n");
}

