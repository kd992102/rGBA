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
    cpu->Cmode = 0x1F;
    cpu->Halt = 0;
    //getchar();
    for(;;){
        if(cpu->Halt == 0){
            cpu->Cmode = ChkCPUMode();
            cpu->CurrentInst = cpu->fetchcache[2];
            CpuExecute(cpu->fetchcache[2]);

            if(cpu->dMode == THUMB_MODE)cpu->InstOffset = 0x2;
            else{cpu->InstOffset = 0x4;}
            RecoverReg(cpu->Cmode);
            cpu->Reg[PC] += cpu->InstOffset;
            PreFetch(cpu->Reg[PC]);//fetch new instruction
        }
        IRQ_checker(cpu->CPSR);
        if(cpu->Halt == 1)cpu->cycle = 1;
        for(int i=0;i<cpu->cycle;i++){
            cpu->cycle_sum += 1;
            if((cpu->cycle_sum - 393) % 4 == 0)PPU_update((cpu->cycle_sum - 393), texture, renderer);
        }
        /*0x6C6->, 0x10E4->535536 0x1000->879168, 1404563, 1405107, 1731063->0x18, 1731857 interrupt*/
        //cpu->cycle_sum >= 1731891 (0x733b)
        //CurrentInst == 0xE5CC3208,CurrentInst == 0x8118
        //0xa0 cycle:818 -> OK
        //0xbb4 tst r0,0xe000000
        //1730918, 515011
        /*if(cpu->cycle_sum >= 9000000){
            CpuStatus();
            printf("Cycle:%d\n", cpu->cycle_sum);
            printf("Current Instruction:%x\n", cpu->CurrentInst);
            printf("Instruction Cycle:%d\n", cpu->cycle);
            printf("DISP:%x:%x\n", DISPSTAT, MemRead32(DISPSTAT));
            printf("VCOUNT:%x\n", MemRead16(VCOUNT));
            printf("CPSR->I-flag:%x\n", (cpu->CPSR >> 7) & 0x1);
            printf("IME:%x:%x\n", 0x4000208, MemRead8(0x4000208));
            printf("IE:%x:%x\n", 0x4000200, MemRead16(0x4000200));
            printf("IR:%x:%x\n", 0x4000202, MemRead16(0x4000202));
            printf("WRAM %x:%x\n", 0x3007ffc, MemRead32(0x3007ffc));
            printf("Sprite %x:%x\n", 0x7000030, MemRead32(0x7000030));
            printf("WRAM:%x:%x\n", 0x3001568, MemRead32(0x3001568));
            //getchar();
            //exit(1);
        }*/
        cpu->cycle = 0;
    }
    Release_GbaMem();
    free(cpu);
    cpu = NULL;
    return 0;
}
