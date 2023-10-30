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

static SDL_Renderer* renderer;
static SDL_Window* window;
static SDL_Texture* texture;

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

    rom = fopen(rom_file_name, "rb");
    fseek(rom, 0, SEEK_END);
    rom_size = ftell(rom);
    printf("Rom File size : %u bytes\n", rom_size);
    fseek(rom, 0, SEEK_SET);

    if(fread(Mem->Game_ROM1, sizeof(uint8_t), rom_size, rom)){
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
    printf("--->%x\n", Mem->Game_ROM1[0]);
    uint16_t sample = 0x0;
    /*for(uint32_t i=0; i < 0x2000000; i+=2){
        MemWrite16(0x8000000 + i, sample);
        sample += 1;
    }*/
    //memcpy(Mem->Game_ROM1, rom, 0x2000000);
    memcpy(Mem->Game_ROM2, Mem->Game_ROM1, 0x2000000);
    memcpy(Mem->Game_ROM3, Mem->Game_ROM1, 0x2000000);
    
    cpu->Reg[PC] = 0x00000000;//ROM File Reg[PC]
    InitCpu(cpu->Reg[PC]);

    //PPUInit(renderer, window, texture);
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
    }
    if (SDL_CreateWindowAndRenderer(308, 228, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    }
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0x00);
    SDL_RenderClear(renderer);
    PPUMemWrite16(VCOUNT, 0x7E);
    cpu->cycle = 0;
    cpu->cycle_sum = 0;
    uint8_t Cmode = 0x1F;
    uint32_t CurrentInst;
    for(;;){
        Cmode = ChkCPUMode();
        CurrentInst = cpu->fetchcache[2];
        CpuExecute(cpu->fetchcache[2]);

        if(cpu->dMode == THUMB_MODE)cpu->InstOffset = 0x2;
        else{cpu->InstOffset = 0x4;}

        RecoverReg(Cmode);
        cpu->Reg[PC] += cpu->InstOffset;
        PreFetch(cpu->Reg[PC]);//fetch new instruction
        //cpu->cycle_sum += cpu->cycle;
        //cpu->cycle+=1;
        for(int i=0;i<cpu->cycle;i++){
            cpu->cycle_sum += 1;
            if(cpu->cycle_sum % 4 == 0)PPU_update(cpu->cycle_sum, texture, renderer);
        }
        /*0x6C6->, 0x10E4->535536 0x1000->879168, 1404563, 1405107*/
        //cpu->cycle_sum >= 1731999
        //CurrentInst == 0xE5CC3208
        if(cpu->cycle_sum >= 1731895){
            CpuStatus();
            printf("Cycle:%d\n", cpu->cycle_sum);
            printf("Current Instruction:%x\n", CurrentInst);
            printf("Instruction Cycle:%d\n", cpu->cycle);
            printf("DISP:%x:%x\n", DISPSTAT, MemRead32(DISPSTAT));
            printf("WRAM %x:%x\n", 0x3007ff0, MemRead32(0x3007ff0));
            //printf("Palette %x:%x\n", 0x5000038, MemRead32(0x5000038));
            //printf("BG %x:%x\n", 0x6000300, MemRead32(0x6000300));
            //printf("OBJ %x:%x\n", 0x6016c92, MemRead32(0x6016c92));
            printf("Sprite %x:%x\n", 0x7000030, MemRead32(0x7000030));
            printf("WRAM:%x:%x\n", 0x3001568, MemRead32(0x3001568));
            getchar();
            //exit(1);
        }
        cpu->cycle = 0;
    }

    //Video Test Loop
    /*for(;;){
        PPU_update(cpu->cycle, texture, renderer);
        cpu->cycle += 1;
    }*/

    Release_GbaMem();
    free(cpu);
    cpu = NULL;
    return 0;
}
