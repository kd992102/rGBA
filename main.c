#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
#include "dma.h"
#include "gba.h"
#include "ppu.h"

SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;

//struct gba_rom *gba_rom;
FILE *bios;
FILE *rom;

Gba_Cpu *cpu;
GbaMem *Mem;
DMA DMA_CH;

uint32_t bios_size;
uint32_t rom_size;
static const char bios_file_name[] = "gba_bios.bin";
static const char rom_file_name[] = "pok_g_386.gba";

int main(int argc, char *argv[]){

    bios = fopen(bios_file_name, "rb");
    cpu = malloc(sizeof(Gba_Cpu));
    Mem = malloc(sizeof(GbaMem));
    cpu->CpuMode = SYSTEM;
    cpu->dMode = ARM_MODE;
    printf("start\n");
    Init_GbaMem();

    fseek(bios, 0, SEEK_END);
    bios_size = ftell(bios);
    printf("Bios File size : %u bytes\n", bios_size);
    fseek(bios, 0, SEEK_SET);
    
    if(fread(Mem->BIOS, sizeof(uint8_t), bios_size, bios)){
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
    uint16_t sample = 0x0;
    for(uint32_t i=0; i < 0x2000000; i+=2){
        MemWrite16(0x8000000 + i, sample);
        sample += 1;
    }
    memcpy(Mem->Game_ROM2, Mem->Game_ROM1, 0x2000000);
    memcpy(Mem->Game_ROM3, Mem->Game_ROM1, 0x2000000);
    
    cpu->Reg[PC] = 0x00000000;//ROM File Reg[PC]
    InitCpu(cpu->Reg[PC]);

    PPUInit(renderer, window, texture);

    cpu->cycle = 0;
    uint8_t Cmode = 0x1F;
    for(;;){
        Cmode = ChkCPUMode();

        CpuExecute(cpu->fetchcache[2]);

        if(cpu->dMode == THUMB_MODE)cpu->InstOffset = 0x2;
        else{cpu->InstOffset = 0x4;}

        RecoverReg(Cmode);
        cpu->Reg[PC] += cpu->InstOffset;
        PreFetch(cpu->Reg[PC]);//fetch new instruction
        PPU_update(cpu->cycle);
        /*516280 b 0x868 1382277 0x4000004 content d2 1363820 1390712 1401107 1416598 1600385*/
        if(cpu->cycle >= 1417662){
            CpuStatus();
            printf("Cycle:%d\n", cpu->cycle);
            uint32_t test = 0x4000000;
            printf("%08x : %08x\n", test, MemRead32(test));
            printf("%08x : %08x\n", test + 0x4, MemRead32(test + 0x4));
            printf("%08x : %08x\n", test + 0x8, MemRead32(test + 0x8));
            printf("%08x : %08x\n", test + 0xc, MemRead32(test + 0xc));
            getchar();
        }
    }


    Release_GbaMem();
    free(cpu);
    cpu = NULL;
    return 0;
}
