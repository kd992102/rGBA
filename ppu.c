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

struct OBJ_Affine * LoadOBJAffine(struct OBJ_Affine * affine, uint16_t GroupNum){
    affine->PA = MemRead16(0x7000006 + 0x20 * GroupNum);
    affine->PB = MemRead16(0x700000E + 0x20 * GroupNum);
    affine->PC = MemRead16(0x7000016 + 0x20 * GroupNum);
    affine->PD = MemRead16(0x700001E + 0x20 * GroupNum);
    return affine;
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

void delay(int milli_seconds)
{
    // Converting time into milli_seconds
    int seconds = 10 * milli_seconds;
 
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + seconds);
}

void DrawBG(SDL_Renderer *renderer, uint16_t vcount, uint16_t h){
    
}

void DrawSprite(SDL_Renderer* renderer, uint16_t vcount, uint16_t h){
    uint32_t Display = MemRead16(DISPCNT);
    struct OBJ_Affine * affine;
    affine = malloc(sizeof(struct OBJ_Affine));
    uint8_t VideoMode = (Display & 0x7); //Video Mode 0-5
    int8_t BGpriority, BGindex;
    uint16_t BGControl[4], BGHofs[4], BGVofs[4];
    OAM_attr OBJattr;
    uint8_t Color, Mosaic, Shape, RSFlag, DoubleFlag, OBJMode;
    int16_t Xcoord;
    int8_t Ycoord;
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
        RSFlag = ((OBJattr.at0 >> 8) & 0x1);
        Xcoord = (OBJattr.at1 & 0x1ff);
        Ycoord = (OBJattr.at0 & 0xff);
        TileNum = (OBJattr.at2 & 0x3ff);
        Mosaic = (OBJattr.at0 >> 12) & 0x1;
        Shape = (OBJattr.at0 >> 14) & 0x3;
        Color = (OBJattr.at0 >> 13) & 0x1;
        if(Color == 0)(OBJattr.at2 >> 12) & 0xf;
        OBJSize = (OBJattr.at1 >> 14) & 0x3;
        Priority = (OBJattr.at2 >> 12) & 0xf;
        DoubleFlag = (OBJattr.at0 >> 9) & 0x1;
        if(RSFlag){
            RSparam = (OBJattr.at1 >> 9) & 0x1f;
            Disable = 0;
            affine = LoadOBJAffine(affine, RSparam);
        }
        else{Disable = (OBJattr.at0 >> 9) & 0x1;}
        HFlip = (OBJattr.at1 >> 12) & 0x1;
        VFlip = (OBJattr.at1 >> 13) & 0x1;
        if(Disable == 1)continue;
        if(vcount == Ycoord && h == Xcoord){
            TileAddr = 0x6010000 + ((OBJattr.at2 & 0x3ff) * 32);
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
            OBJtile.h = ObjV;
            OBJtile.w = ObjH;
            OBJtile.x = Xcoord;
            OBJtile.y = Ycoord;
            if(Xcoord >= 0x100)Xcoord = (int16_t)(Xcoord - 0x1FF);
            
            //printf("X:%d, Y:%d\n", Xcoord, Ycoord);
            uint16_t *array;//64 pixels = 1 tile
            array = malloc(sizeof(uint16_t) * ObjH * ObjV);
            //x2 = A(x1-x0) + B(y1-y0) + x0
            //y2 = C(x1-x0) + D(y1-y0) + y0
            
            if(RSFlag){
                OBJtile.y = Ycoord;
                OBJtile.x = Xcoord;
                for(uint8_t spry=0;spry<ObjV;spry+=8){
                    for(uint8_t j=0;j<8;j++){
                        for(uint8_t sprx=0;sprx<ObjH;sprx+=8){
                            for(uint8_t i=0;i<8;i++){
                                array[((spry + j) * ObjH) + sprx + i] = MemRead8(TileAddr + 0x400*(spry/8) + 0x40*(sprx/8) + j*8 + i);
                                if(Color){
                                    array[((spry + j) * ObjH) + sprx + i] = MemRead16(0x5000200+0x2*array[((spry + j) * ObjH) + sprx + i]);
                                }
                                else{
                                    array[((spry + j) * ObjH) + sprx + i] = MemRead16(0x5000200+0x10*(((spry + j) * ObjH) + sprx + i & 0xf) + ((((spry + j) * ObjH) + sprx + i >> 4) & 0xf));
                                }
                                if(array[((spry + j) * ObjH) + sprx + i]==0)array[((spry + j) * ObjH) + sprx + i] = 0x7fff;
                            }
                        }
                    }
                }
                SDL_Surface *Surf = SDL_CreateRGBSurfaceWithFormatFrom(array,ObjH,ObjV,16,2*ObjH,SDL_PIXELFORMAT_BGR555);
                SDL_SetColorKey(Surf, SDL_TRUE, SDL_MapRGB(Surf->format, 255, 255, 255));
                SDL_Texture *Tex = SDL_CreateTextureFromSurface(renderer, Surf);
                SDL_RenderCopy(renderer, Tex, NULL, &OBJtile);
                SDL_RenderPresent(renderer);
                SDL_FreeSurface(Surf);
            }
            else{
                OBJtile.y = Ycoord;
                OBJtile.x = Xcoord;
                for(uint8_t spry=0;spry<ObjV;spry+=8){
                    for(uint8_t j=0;j<8;j++){
                        for(uint8_t sprx=0;sprx<ObjH;sprx+=8){
                            for(uint8_t i=0;i<8;i++){
                                //getchar();
                                array[((spry + j) * ObjH) + sprx + i] = MemRead8(TileAddr + 0x400*(spry/8) + 0x40*(sprx/8) + j*8 + i);
                                //printf("TileAddr:%x\n", TileAddr + 0x400*(spry/8) + 0x40*(sprx/8) + j*8 + i);
                                if(Color){
                                    array[((spry + j) * ObjH) + sprx + i] = MemRead16(0x5000200+0x2*array[((spry + j) * ObjH) + sprx + i]);
                                }
                                else{
                                    array[((spry + j) * ObjH) + sprx + i] = MemRead16(0x5000200+0x10*(((spry + j) * ObjH) + sprx + i & 0xf) + ((((spry + j) * ObjH) + sprx + i >> 4) & 0xf));
                                }
                                if(array[((spry + j) * ObjH) + sprx + i]==0)array[((spry + j) * ObjH) + sprx + i] = 0x7fff;
                            }
                        }
                    }
                }
                SDL_Surface *Surf = SDL_CreateRGBSurfaceWithFormatFrom(array,ObjH,ObjV,16,2*ObjH,SDL_PIXELFORMAT_BGR555);
                SDL_SetColorKey(Surf, SDL_TRUE, SDL_MapRGB(Surf->format, 255, 255, 255));
                SDL_Texture *Tex = SDL_CreateTextureFromSurface(renderer, Surf);
                SDL_RenderCopy(renderer, Tex, NULL, &OBJtile);
                SDL_RenderPresent(renderer);
                SDL_FreeSurface(Surf);
            }
            //printf("Sprite ID:%d, H:%d, W:%d\n", ((OAM_ADDR - OAM_ADDR_BASE) / 8), ObjH, ObjV);
            free(array);
            array = NULL;
        }
    }
}

void DrawScanLine(uint16_t vcount, SDL_Texture* texture, SDL_Renderer* renderer){
    uint32_t Display = MemRead16(DISPCNT);
    uint8_t OBJenable = (Display >> 12);
    
    for(uint16_t h=0;h<0x200;h++){
        if(OBJenable){
            DrawSprite(renderer, vcount, h);
        }
        else{
            break;
        }
    }
}

/*void DrawScreen(SDL_Renderer *renderer, char screen[0xE4][0x200]){
    
}*/

void PPU_update(uint32_t cycle, SDL_Texture* texture, SDL_Renderer* renderer){
    uint16_t reg_vcount = MemRead8(VCOUNT);
    uint16_t reg_status = MemRead16(DISPSTAT);
    uint8_t reg_hblank = 0;
    uint8_t reg_vblank = 0;
    uint8_t reg_vc_match = 0;
    uint8_t disp_vcount = (reg_status >> 8) & 0xff;
    uint32_t horizon = 0;
    uint32_t vertical = 0;
    horizon = cycle % SCANLINE_CYCLE;
    if(reg_vcount == disp_vcount){
        reg_vc_match = 1;
        //printf("v-count match, reg:%d, disp:%d\n", reg_vcount, disp_vcount);
    }
    else{reg_vc_match = 0;}

    if(reg_vcount >= VDRAW && reg_vcount < 228){
        reg_vblank = 1;
        //DMA_Transfer(DMA_CH, 1);
    }
    else{reg_vblank = 0;}

    if(horizon > HBLANK_ZERO_CYCLE){
        reg_hblank = 1;
        //DMA_Transfer(DMA_CH, 2);
    }
    else{
        reg_hblank = 0;
    }
    if(horizon == 0){
        if(reg_vcount < VDRAW ){
            DrawScanLine(reg_vcount, texture, renderer);
        }
        reg_vcount += 1;
        if(reg_vcount == 228){
            reg_vcount = 0;
            /*SDL_SetRenderTarget(renderer, NULL);
            SDL_SetRenderDrawColor(renderer, 255,255,255,0);
            SDL_RenderClear(renderer);*/
        }
        PPUMemWrite16(VCOUNT, reg_vcount);
    }

    reg_status = ((reg_status & 0xfff8) | (reg_vc_match << 2) | (reg_hblank << 1) | (reg_vblank));
    PPUMemWrite16(DISPSTAT, reg_status);
    reg_status = MemRead16(DISPSTAT);
    if(((reg_status >> 3) & 1) && (reg_vcount == 0xA0)){
        PPUMemWrite16(0x4000202, 0x1);
    }
    else if(((reg_status >> 4) & 1) && ((reg_status >> 1) & 1) && (MemRead8(0x4000208) == 0))PPUMemWrite16(0x4000202, 0x2);
    else if(((reg_status >> 5) & 1) && ((reg_status >> 2) & 1) && (MemRead8(0x4000208) == 0))PPUMemWrite16(0x4000202, 0x3);
}