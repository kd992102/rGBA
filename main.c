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
    /*for(;;){
        //Run 1 Frame Each Loop
    }*/
    uint32_t instruction = MemRead32(cpu->Reg[PC]);
    printf("instruction:%x\n", instruction);
    Release_GbaMem();
    free(cpu);
    cpu = NULL;
    return 0;
}