#include <stdio.h>
#include <time.h>
#include "memory.h"
#include "io.h"
#include "dma.h"
#include "ppu.h"


extern GbaMem *Mem;
extern DMA DMA_CH;

uint8_t v_count = 0;
uint16_t h = 0;
uint8_t v = 0;
uint8_t bg_enable_mask[3] = {0xf, 0x7, 0xc};

void DbgWindow(){

}

void PPUInit(SDL_Renderer* renderer, SDL_Window* window, SDL_Texture* texture){
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
    }
    if (SDL_CreateWindowAndRenderer(308, 228, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    }
    //SDL_SetWindowResizable(window, SDL_TRUE);
    texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB1555,SDL_TEXTUREACCESS_STREAMING,8,8);
}

void DrawLine(SDL_Renderer* renderer, SDL_Texture *texture, uint32_t v){
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderDrawLine(renderer, 0, v, 239, v);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void DrawPixel(SDL_Renderer* renderer, uint8_t color, uint32_t x, uint32_t y){
    uint8_t r,g,b;
    r = (color >> 10) & 0x1f;
    g = (color >> 5) & 0x1f;
    b = (color & 0x1f);
    SDL_SetRenderDrawColor(renderer, r, g, b, 0x00);
    SDL_RenderDrawPoint(renderer, x, y);
    SDL_RenderPresent(renderer);
}

void delay(int milli_seconds)
{
    // Converting time into milli_seconds
    int seconds = 10 * milli_seconds;
 
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + seconds);
}

void DrawSprite(SDL_Renderer* renderer, uint16_t vcount, uint16_t h){
    uint32_t Display = MemRead16(DISPCNT);
    uint8_t VideoMode = (Display & 0x7); //Video Mode 0-5
    int8_t BGpriority, BGindex;
    uint16_t BGControl[4], BGHofs[4], BGVofs[4];
    OAM_attr OBJattr;
    uint8_t Xcoord, Ycoord, Color, Mosaic, Shape, RSFlag, DoubleFlag, OBJMode;
    uint8_t HFlip, VFlip, OBJSize, RSparam, Priority, Disable;
    uint16_t TileNum;
    uint32_t TileAddr;
    SDL_Rect Tile;
    uint8_t ObjH, ObjV;
    Tile.h = 8;
    Tile.w = 8;
    SDL_Rect OBJtile;
    uint32_t Sprite[128];
    for(uint32_t OAM_ADDR = OAM_ADDR_BASE;OAM_ADDR < (OAM_ADDR_BASE + (128*8));OAM_ADDR+=8){
        OBJattr.at0 = MemRead16(OAM_ADDR);
        OBJattr.at1 = MemRead16(OAM_ADDR+0x2);
        OBJattr.at2 = MemRead16(OAM_ADDR+0x4);
        OBJattr.at3 = MemRead16(OAM_ADDR+0x8);
        if(OBJattr.at0 == 0 && OBJattr.at1 == 0 && OBJattr.at2 == 0 && OBJattr.at3 == 0)break;
        RSFlag = (OBJattr.at0 >> 8) & 0x1;
        Xcoord = OBJattr.at1 & 0xff;
        Ycoord = OBJattr.at0 & 0xff;
        TileNum = (OBJattr.at2 & 0x3ff);
        Mosaic = (OBJattr.at0 >> 12) & 0x1;
        Shape = (OBJattr.at0 >> 14) & 0x3;
        Color = (OBJattr.at0 >> 13) & 0x1;
        if(Color == 0)(OBJattr.at2 >> 12) & 0xf;
        OBJSize = (OBJattr.at1 >> 14) & 0x3;
        Priority = (OBJattr.at2 >> 12) & 0xf;
        if(RSFlag){
            DoubleFlag = (OBJattr.at0 >> 9) & 0x1;
            RSparam = (OBJattr.at1 >> 9) & 0x1f;
        }
        else{
            Disable = (OBJattr.at0 >> 9) & 0x1;
            HFlip = (OBJattr.at1 >> 12) & 0x1;
            VFlip = (OBJattr.at1 >> 13) & 0x1;
            if(Disable == 0 && vcount == Ycoord && h == Xcoord){
                TileAddr = 0x6010000 + ((OBJattr.at2 & 0x3ff) * 32);
                //printf("TileAddr:%x\n", TileAddr);
                switch(Shape){
                    case 0:
                        switch(OBJSize){
                            case 0:
                                ObjH = 8;
                                ObjV = 8;
                                break;
                            case 1:
                                ObjH = 16;
                                ObjV = 16;
                                break;
                            case 2:
                                ObjH = 32;
                                ObjV = 32;
                                break;
                            case 3:
                                ObjH = 64;
                                ObjV = 64;
                                break;
                        }
                        break;
                    case 1:
                        switch(OBJSize){
                            case 0:
                                ObjH = 16;
                                ObjV = 8;
                                break;
                            case 1:
                                ObjH = 32;
                                ObjV = 8;
                                break;
                            case 2:
                                ObjH = 32;
                                ObjV = 16;
                                break;
                            case 3:
                                ObjH = 64;
                                ObjV = 32;
                                break;
                        }
                        break;
                    case 2:
                        switch(OBJSize){
                            case 0:
                                ObjH = 8;
                                ObjV = 16;
                                break;
                            case 1:
                                ObjH = 8;
                                ObjV = 32;
                                break;
                            case 2:
                                ObjH = 16;
                                ObjV = 32;
                                break;
                            case 3:
                                ObjH = 32;
                                ObjV = 64;
                                break;
                        }
                        break;
                }
                OBJtile.h = 8;
                OBJtile.w = 8;
                OBJtile.x = Xcoord;
                OBJtile.y = Ycoord;
                uint16_t array[64];//64 pixels = 1 tile
                printf("H:%d,V:%d\n", ObjH, ObjV);
                for(int spry=0;spry<(ObjV/8);spry+=1){
                    OBJtile.y = Ycoord + (spry*8);
                    OBJtile.x = Xcoord;
                    for(int sprx=0;sprx<(ObjH/8);sprx+=1){
                        OBJtile.x = Xcoord + (sprx*8);
                        for(int i=0;i<64;i++){//a tile
                            array[i] = MemRead8(TileAddr + 0x400*spry + 0x40*sprx + i);
                            //printf("addr:%x->content:%x\n", TileAddr + 0x400*spry + 0x40*sprx + i, array[i]);
                            //printf("addr:%x->content:%x\n", 0x6016c92, MemRead8(0x6016c92));
                            array[i] = MemRead16(0x5000200+0x2*array[i]);
                        }
                        //getchar();
                        SDL_Surface *Surf = SDL_CreateRGBSurfaceWithFormatFrom(array,8,8,16,2*8,SDL_PIXELFORMAT_BGR555);
                        SDL_Texture *Tex = SDL_CreateTextureFromSurface(renderer, Surf);
                        SDL_Rect Rect;
                        SDL_RenderCopy(renderer, Tex, NULL, &OBJtile);
                        SDL_RenderPresent(renderer);
                    }
                }
                continue;
            }
            else if(Disable == 1){
                continue;
            }
        }
    }
}

void DrawScanLine(uint16_t vcount, SDL_Texture* texture, SDL_Renderer* renderer){
    uint32_t Display = MemRead16(DISPCNT);
    uint8_t OBJenable = (Display >> 12);
    for(int h=0;h<308;h++){
        if(OBJenable){
            DrawSprite(renderer, vcount, h);
        }
        else{
            break;
        }
    }
    /*void *scanline;
    uint32_t Pitch = 240 * 4;
    SDL_LockTexture(texture, NULL, &scanline, &Pitch);
    for(uint8_t i = 0;i < 4;i++){
        BGControl[i] = MemRead16(BG0CNT + 2*i);
    }
    for(uint8_t i = 0;i < 4;i++){
        BGHofs[i] = MemRead16(BG0HOFS + 4*i);
    }
    for(uint8_t i = 0;i < 4;i++){
        BGVofs[i] = MemRead16(BG0VOFS + 4*i);
    }
    printf("DISPCNT:%x\n", MemRead32(DISPCNT));
    switch(VideoMode){
        //Tile/Map based Modes
        case 0:
        case 1:
        case 2:
            uint8_t enable_bg = (Display >> 8) & bg_enable_mask[VideoMode];
            //if(VideoMode == 2)printf("Video Mode %d enable bg %08x\n", VideoMode, (Display >> 8));
            for(BGpriority=3;BGpriority >= 0;BGpriority--){
                //printf("Drawing...\n");
                for(BGindex=3;BGindex >= 0;BGindex--){
                    if(!(enable_bg & (1 << BGindex))) continue;
                    printf("For Loop\n");
                    if((BGControl[BGindex] & 0x3) != BGpriority) continue;
                    uint32_t Chr_Base = ((BGControl[BGindex] >> 2) & 0x3) << 14;
                    uint8_t ColorDep = ((BGControl[BGindex] >> 7) & 0x1);
                    uint16_t Scrn_Base = ((BGControl[BGindex] >> 8) & 0x1f) << 11;
                    uint32_t Aff_Wrap = ((BGControl[BGindex] >> 13) & 0x1);
                    uint32_t Scrn_Size = (BGControl[BGindex] >> 14);
                    uint8_t affine = 0;
                    if(VideoMode == 2 || (VideoMode == 1 && BGindex == 2)) affine = 1;

                    uint16_t Oy = vcount + BGVofs[BGindex];
                    uint16_t Tmy = Oy >> 3;
                    uint16_t Scrn_Y = (Tmy >> 5) & 1;
                    uint8_t h_draw;
                    uint32_t Addr = vcount * 240 * 4;
                    
                    for(h_draw = 0;h_draw < HDRAW;h_draw++){
                        uint16_t Ox = h_draw + BGHofs[BGindex];
                        uint16_t Tmx = Ox >> 3;
                        uint16_t Scrn_X = (Tmx >> 5) & 1;

                        uint16_t Chr_X = Ox & 7;
                        uint16_t Chr_Y = Oy & 7;

                        uint16_t Palindex;
                        uint16_t PalBase = 0;

                        uint32_t Map_Addr = Scrn_Base + (Tmy & 0x1F) * 32 * 2 + (Tmx & 0x1f) * 2;
                        switch (Scrn_Size) {
                            case 1: Map_Addr += Scrn_X * 2048; break;
                            case 2: Map_Addr += Scrn_Y * 2048; break;
                            case 3: Map_Addr += Scrn_X * 2048 + Scrn_Y * 4096; break;
                        }
                        printf("Map Addr %08x\n", Map_Addr);
                        uint16_t Tile = MemRead16(Map_Addr) | (MemRead16(Map_Addr + 1) << 8);

                        uint16_t Chr_Numb = (Tile >> 0) & 0x3ff;
                        uint8_t Flip_X = (Tile >> 10) & 0x1;
                        uint8_t Flip_Y = (Tile >> 11) & 0x1;
                        uint8_t Chr_Pal = (Tile >> 12) & 0xf;

                        if(!ColorDep) PalBase = Chr_Pal * 16;
                        if(Flip_X) Chr_X ^= 7;
                        if(Flip_Y) Chr_Y ^= 7;

                        uint32_t VramAddr;
                        if(!ColorDep){
                            VramAddr = Chr_Base + Chr_Numb * 64 + Chr_Y * 8 + Chr_X;
                            Palindex = MemRead16(VramAddr);
                        }
                        else{
                            VramAddr = Chr_Base + Chr_Numb * 32 + Chr_Y * 4 + (Chr_X >> 1);
                            Palindex = (MemRead16(VramAddr) >> (Chr_X & 1) * 4) & 0xf;
                        }

                        uint32_t PalAddr = Palindex | PalBase;
                        if(Palindex) *(uint32_t *)(scanline + Addr) = MemRead32(PalAddr);
                        Addr += 4;
                    }
                }
            }
            break;
    }*/
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void PPU_update(uint32_t cycle, SDL_Texture* texture, SDL_Renderer* renderer){
    uint16_t reg_vcount = MemRead16(VCOUNT);
    uint16_t reg_status = MemRead16(DISPSTAT);
    uint8_t reg_hblank = 0;
    uint8_t reg_vblank = 0;
    uint8_t reg_vc_match = 0;
    uint8_t disp_vcount = (reg_status >> 8) & 0xff;
    uint32_t horizon = 0;
    uint32_t vertical = 0;
    horizon = cycle % SCANLINE_CYCLE;
    vertical = (cycle % (SCANLINE_CYCLE * 228)) / SCANLINE_CYCLE;

    if(reg_vcount == disp_vcount){
        reg_vc_match = 1;
        //printf("v-count match, reg:%d, disp:%d\n", reg_vcount, disp_vcount);
    }
    else{reg_vc_match = 0;}

    if(vertical >= VDRAW){
        reg_vblank = 1;
        //SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0x00);
        //SDL_RenderClear(renderer);
        DMA_Transfer(DMA_CH, 1);
    }
    else if(vertical == 0){reg_vblank = 0;}

    if(horizon >= HBLANK_ZERO_CYCLE){
        reg_hblank = 1;
        DMA_Transfer(DMA_CH, 2);
    }
    else{
        if(horizon == 0 && vertical < VDRAW)reg_hblank = 0;
    }

    if(horizon == 0){
        if(vertical < VDRAW ){
            //printf("Line : %d\n", vertical);
            DrawScanLine(vertical, texture, renderer);
            //delay(1);
        }
        MemWrite16(VCOUNT, vertical);
    }

    reg_status = reg_status | (reg_vc_match << 2) | (reg_hblank << 1) | (reg_vblank);
    MemWrite16(DISPSTAT, reg_status);
}