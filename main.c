#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
#include "dma.h"
#include "gba.h"
#include "timer.h"
#include "ppu.h"

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
    void *screen;
    screen = malloc(sizeof(uint32_t) * 240 * 4 * 160);
    printf("screen %x\n", screen);

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
    uint16_t sample = 0x0;

    memcpy(Mem->Game_ROM2, Mem->Game_ROM1, 0x2000000);
    memcpy(Mem->Game_ROM3, Mem->Game_ROM1, 0x2000000);
    
    cpu->Reg[PC] = 0x00000000;//ROM File Reg[PC]
    InitCpu(cpu->Reg[PC]);

    SDL_Renderer* renderer;
    SDL_Window* window;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
    }
    if (SDL_CreateWindowAndRenderer(240, 160, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    }

    SDL_Texture* texture;
    int32_t tex_pitch = 240*4;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, 240, 160);
    SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x00);
    SDL_RenderClear(renderer);
    PPUMemWrite16(VCOUNT, 0x7E);
    //getchar();
    SDL_LockTexture(texture, NULL, &screen, &tex_pitch);
    for(uint32_t addr = 0;addr < (159*240*4);addr += 0x10){
        *(uint32_t *)(screen + (addr | 0x0)) = ColorFormatTranslate(0x7fff);
        *(uint32_t *)(screen + (addr | 0x4)) = ColorFormatTranslate(0x7fff);
        *(uint32_t *)(screen + (addr | 0x8)) = ColorFormatTranslate(0x7fff);
        *(uint32_t *)(screen + (addr | 0xc)) = ColorFormatTranslate(0x7fff);
    }
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    //getchar();

    //main loop
    for(;;){
        if(cpu->Halt == 0){
            cpu->Cmode = ChkCPUMode();
            cpu->CurrentInst = cpu->fetchcache[2];
            CpuExecute(cpu->fetchcache[2]);
            if(cpu->dMode == THUMB_MODE)cpu->InstOffset = 0x2;
            else{cpu->InstOffset = 0x4;}
            cpu->Reg[PC] += cpu->InstOffset;
            PreFetch(cpu->Reg[PC]);//fetch new instruction
            //According to the instruction cycles update the PPU 
            cpu->frame_cycle += cpu->cycle;
            if(cpu->frame_cycle >= 1232){
                if(cpu->cycle_sum >= 393)PPU_update(texture, renderer, screen);
                cpu->cycle_sum += cpu->frame_cycle;
                cpu->frame_cycle = 0;
            }
            //cpu->cycle_sum >= 76122822、76121010、1730674、2006941、1730860、76385095、75852696、75326510
            if(cpu->fetchcache[2] == 0xe92d500f && cpu->cycle_sum >= 75000000){
                CpuStatus();
                printf("cycle:%ld\n", cpu->cycle_sum);
                printf("IME:%x, IE:%x, IF:%x\n", MemRead8(0x4000208), MemRead8(0x4000200), MemRead8(0x4000202));
                getchar();
            }
            RecoverReg(cpu->Cmode);
        }
        else if(cpu->Halt == 1){
            cpu->frame_cycle += 1;
            if(cpu->frame_cycle >= 1232){
                PPU_update(texture, renderer, screen);
                cpu->cycle_sum += cpu->frame_cycle;
                cpu->frame_cycle = 0;
            }
        }
        IRQ_checker(cpu->CPSR);
        //if Halt enable
        Timer_Clock(cpu->cycle);
        cpu->cycle = 0;
    }
    Release_GbaMem();
    free(cpu);
    cpu = NULL;
    return 0;
}