#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include "typedef.h"
#include "arm7tdmi.h"
//#include "cart.h"


Gba_Cpu *cpu;
//struct gba_rom *gba_rom;
FILE *bios;
FILE *rom;
uint32_t bios_size;
uint32_t rom_size;
static const char bios_file_name[] = "gba_bios.bin";
static const char rom_file_name[] = "pok_g_386.gba";

int main(int argc, char *argv[]){
    /*
    for(int addr=0;addr<128;addr+=4){
        printf("0x%08x ", *((u32 *)(&cpu.Mem[addr])));
        cpu.reg[15] = addr + 0x8;
        ArmInstDec(*((u32 *)(&cpu.Mem[addr])));
    }*/
    bios = fopen(bios_file_name, "rb");
    cpu = malloc(sizeof(Gba_Cpu));
    printf("Gba_Cpu:0x%08x\n", cpu);
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

    rom = fopen(rom_file_name, "rb");
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
    }
    cpu->Ptr = 0x00000000;//First ptr
    InitCpu(cpu, 0x00000000);
    for(int addr=0x00000000;addr<0x00000040;addr+=4){
        //Execute
        //Decode
        //Prefetch
        if(cpu->dMode == THUMB_MODE)cpu->InstOffset = 0x2;
        else{cpu->InstOffset = 0x4;}
        printf("0x%08x, 0x%08x, 0x%08x\n", cpu->Ptr, cpu->Reg[PC], cpu->fetchcache[2]);
        //Execute
        CpuDecode(cpu, cpu->fetchcache[2]);
        cpu->Ptr += cpu->InstOffset;
        //Prefetch
        PreFetch(cpu, cpu->Ptr);
    }

    Release_GbaMem(cpu->GbaMem);
    free(cpu);
    cpu = NULL;
    return 0;
}