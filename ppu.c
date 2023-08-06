#include <stdio.h>
#include "memory.h"
#include "io.h"
#include "dma.h"
#include "ppu.h"

extern GbaMem *Mem;
extern DMA DMA_CH;

uint8_t v_count = 0;
uint16_t h = 0;
uint8_t v = 0;

void PPUInit(SDL_Renderer* renderer, SDL_Window* window, SDL_Texture* texture){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_CreateWindowAndRenderer(240, 160, 0, &window, &renderer);
    SDL_SetWindowSize(window, 240, 160);
    SDL_SetWindowResizable(window, SDL_TRUE);
}

void DrawPixel(uint32_t cycle){

}

void DrawScanLine(uint32_t cycle){
    /*while(cycle < ((v_count + 1) * SCANLINE_CYCLE) && cycle >= (v_count * SCANLINE_CYCLE)){
        if()
    }*/
}

void PPU_update(uint32_t cycle){
    uint16_t vcount = MemRead16(VCOUNT);
    uint16_t status = MemRead16(DISPSTAT);
    uint8_t hblank = (status >> 1) & 0x1;
    uint8_t vblank = (status) & 0x1;

    //match
    if(vcount == ((status >> 8) & 0xff) && cycle >= (SCANLINE_CYCLE + 3)){
        status |= V_COUNT_FLAG;
        MemWrite16(DISPSTAT, status);
    }
    else{
        status &= 0xfffb;
        MemWrite16(DISPSTAT, status);
    }

    if(vcount >= VDRAW){
        //printf("VBLANK vcount:%d\n", vcount);
        status |= V_BLANK_FLAG;
        MemWrite16(DISPSTAT, status);
        if(vcount == VDRAW)DMA_Transfer(DMA_CH, 1);
    }
    else{
        status &= 0xfffe;
        MemWrite16(DISPSTAT, status);
    }

    if(((cycle - 3) % SCANLINE_CYCLE) >= HBLANK_ZERO_CYCLE){
        status |= H_BLANK_FLAG;
        MemWrite16(DISPSTAT, status);
        hblank = (status >> 1) & 0x1;
        if(hblank == 1 && ((cycle - 3) % SCANLINE_CYCLE) == HBLANK_ZERO_CYCLE)DMA_Transfer(DMA_CH, 2);
    }
    else{
        status &= 0xfffd;
        MemWrite16(DISPSTAT, status);
    }

    if((((cycle - 3) / SCANLINE_CYCLE) % 228) == 0){
        vcount = 0;
        MemWrite16(VCOUNT, 0);
    }
    else{
        if((((cycle - 3) / SCANLINE_CYCLE) % 228) > vcount){
            //printf("current %d, vcount %d\n",((cycle - 3) / SCANLINE_CYCLE) % 228, vcount);
            MemWrite16(VCOUNT, vcount + 1);
        }
    }
}