#include <stdio.h>
#include <time.h>
#include "memory.h"
#include "io.h"
#include "dma.h"
#include "ppu.h"


extern GbaMem *Mem;
extern DMA DMA_CH;


int32_t tex_pitch = 240 * 4;
uint8_t v_count = 0;
uint16_t h = 0;
uint8_t v = 0;
uint8_t objwin = 0;
uint8_t bg_enable_mask[3] = {0xf, 0x7, 0xc};

struct BG_CNT LoadBGCNT(struct BG_CNT BGCNT, uint8_t BGNum){
    uint16_t Data;
    switch(BGNum){
        case 0:
            Data = MemRead16(BG0CNT);
            break;
        case 1:
            Data = MemRead16(BG1CNT);
            break;
        case 2:
            Data = MemRead16(BG2CNT);
            break;
        case 3:
            Data = MemRead16(BG3CNT);
            break;
    }
    BGCNT.Prio = (Data & 0x3);
    BGCNT.CharBaseBlock = (Data >> 2) & 0x3;
    BGCNT.Mosaic = (Data >> 6) & 0x1;
    BGCNT.Color = (Data >> 7) & 0x1;
    BGCNT.ScreenBaseBlock = (Data >> 8) & 0x1f;
    BGCNT.DispAreaOverflow = (Data >> 13) & 0x1;
    BGCNT.ScreenSize = (Data >> 14) & 0x3;
    return BGCNT;
}

struct OBJ_Affine * LoadOBJAffine(struct OBJ_Affine * affine, uint16_t GroupNum){
    affine->PA = MemRead16(0x7000006 + 0x20 * GroupNum);
    affine->PB = MemRead16(0x700000E + 0x20 * GroupNum);
    affine->PC = MemRead16(0x7000016 + 0x20 * GroupNum);
    affine->PD = MemRead16(0x700001E + 0x20 * GroupNum);
    return affine;
}

uint32_t ColorFormatTranslate(uint16_t BGR555){
    uint8_t r = ((BGR555 >> 0) & 0x1f) << 3;
    uint8_t g = ((BGR555 >> 5) & 0x1f) << 3;
    uint8_t b = ((BGR555 >> 10) & 0x1f) << 3;
    return (uint32_t)((r | (r >> 5)) << 8) | (uint32_t)((g | (g >> 5)) << 16) | (uint32_t)((b | (b >> 5)) << 24) | (uint32_t)0xff;
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

void DrawOBJwindow(SDL_Renderer* renderer, uint16_t vcount, void *screen){
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
    uint8_t HFlip, VFlip, OBJSize, RSparam, Priority, Disable, chr_pal;
    uint16_t TileNum;
    uint32_t TileAddr;
    int16_t ObjH, ObjV;
    //uint32_t Sprite[128];
    for(uint32_t OAM_ADDR = OAM_ADDR_BASE + (128*8);OAM_ADDR >= (OAM_ADDR_BASE);OAM_ADDR-=8){
        OBJattr.at0 = MemRead16(OAM_ADDR);
        OBJattr.at1 = MemRead16(OAM_ADDR+0x2);
        OBJattr.at2 = MemRead16(OAM_ADDR+0x4);
        OBJattr.at3 = MemRead16(OAM_ADDR+0x8);
        Priority = (OBJattr.at2 >> 10) & 0x3;
        if(OBJattr.at0 == 0 && OBJattr.at1 == 0 && OBJattr.at2 == 0 && OBJattr.at3 == 0)continue;
        RSFlag = ((OBJattr.at0 >> 8) & 0x1);
        if(RSFlag){
            RSparam = (OBJattr.at1 >> 9) & 0x1f;
            Disable = 0;
            affine = LoadOBJAffine(affine, RSparam);
        }
        else{Disable = (OBJattr.at0 >> 9) & 0x1;}
        if(Disable == 1)continue;
        Xcoord = (OBJattr.at1 & 0x1ff);
        Ycoord = (OBJattr.at0 & 0xff);
        OBJMode = (OBJattr.at0 >> 10) & 0x3;
        if(OBJMode != 2)continue;
        objwin = 1;
        TileNum = (OBJattr.at2 & 0x3ff);
        Mosaic = (OBJattr.at0 >> 12) & 0x1;
        Shape = (OBJattr.at0 >> 14) & 0x3;
        Color = (OBJattr.at0 >> 13) & 0x1;
        if(Color == 0)chr_pal = (OBJattr.at2 >> 12) & 0xf;
        OBJSize = (OBJattr.at1 >> 14) & 0x3;
        DoubleFlag = (OBJattr.at0 >> 9) & 0x1;
        HFlip = (OBJattr.at1 >> 12) & 0x1;
        VFlip = (OBJattr.at1 >> 13) & 0x1;
        TileAddr = 0x6010000 + ((OBJattr.at2 & 0x3ff) << 5);
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
        if(Xcoord >= 0x100)Xcoord = (int16_t)(Xcoord - 0x1FF);
        int16_t x_max = 240, y_max = 160;
        if(DoubleFlag){
            ObjV = ObjV*2;
            ObjH = ObjH*2;
            y_max = (Ycoord + ObjV);
            x_max = (Xcoord + ObjH);
        }
        else{
            y_max = (Ycoord + ObjV);
            x_max = (Xcoord + ObjH);
        }
        //if(y_max < Ycoord)Ycoord -= 256;
        int32_t sprx;
        int32_t spry;
        //OBJwin process enable
        if(OBJMode == 2)objwin = 1;

        if(vcount >= Ycoord && vcount < y_max){
            if(RSFlag){
                int32_t rcy = Ycoord + (ObjV>>1);
                int32_t rcx = Xcoord + (ObjH>>1);
                int32_t PA, PB, PC, PD;
                PA = affine->PA;
                PB = affine->PB;
                PC = affine->PC;
                PD = affine->PD;
                int16_t tx;
                int16_t ty;
                
                if(DoubleFlag){
                    rcy = Ycoord + (ObjV>>2);
                    rcx = Xcoord + (ObjH>>2);
                    for(sprx = Xcoord;sprx < x_max;sprx++){
                        if(sprx < 240 && sprx >=0){
                            tx = fx_multiply(PA,(sprx - (Xcoord + (ObjH>>1)))) + fx_multiply(PB,(vcount - (Ycoord + (ObjV>>1)))) + rcx;
                            ty = fx_multiply(PC,(sprx - (Xcoord + (ObjH>>1)))) + fx_multiply(PD,(vcount - (Ycoord + (ObjV>>1)))) + rcy;
                            if(tx < (Xcoord + (ObjH>>1)) && ty < (Ycoord + (ObjV>>1)) && tx >= Xcoord && ty >= Ycoord){
                                uint8_t data = MemRead8(TileAddr + 0x400*((ty - Ycoord)/8) + 0x40 * ((tx - Xcoord)/8) + ((ty - Ycoord) % 8) * 8 + ((tx - Xcoord) % 8));
                                //uint8_t data = 0x1;
                                if(data != 0){
                                    *(uint32_t *)(screen + vcount*240*4 + sprx*4) = 0x1;
                                }
                            }
                        }
                    }
                }
                else{
                    for(sprx = Xcoord;sprx < x_max;sprx++){
                        if(sprx < 240 && sprx >=0){
                            tx = fx_multiply(PA,(sprx - rcx)) + fx_multiply(PB,(vcount - rcy)) + rcx;
                            ty = fx_multiply(PC,(sprx - rcx)) + fx_multiply(PD,(vcount - rcy)) + rcy;
                            if(tx < (Xcoord + ObjH) && ty < (Ycoord + ObjV) && tx >= Xcoord && ty >= Ycoord){
                                uint8_t data = MemRead8(TileAddr + 0x400 * ((ty - Ycoord)/8) + 0x40 * ((tx - Xcoord)/8) + ((ty - Ycoord) % 8) * 8 + ((tx - Xcoord) % 8));
                                if(data != 0){
                                    *(uint32_t *)(screen + vcount*240*4 + sprx*4) = 0x1;
                                }
                            }
                        }
                    }
                }
            }
            else{
                for(sprx = Xcoord;sprx < x_max;sprx++){
                    if((sprx >= 0 && sprx < 240)){
                        uint8_t data = MemRead8(TileAddr + 0x400 * ((vcount - Ycoord) / 8) + 0x40 * ((sprx - Xcoord) / 8) + ((vcount - Ycoord) % 8) * 8 + ((sprx - Xcoord) % 8));
                        if(data != 0){
                            *(uint32_t *)(screen + vcount*240*4 + sprx*4) = 0x1;
                        }
                    }
                }
            }
        }
    }
    //x2 = A(x1-x0) + B(y1-y0) + x0
    //y2 = C(x1-x0) + D(y1-y0) + y0
}

void DrawSprite(SDL_Renderer* renderer, uint8_t prio, uint16_t vcount, void *screen){
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
    uint8_t HFlip, VFlip, OBJSize, RSparam, Priority, Disable, chr_pal;
    uint16_t TileNum;
    uint32_t TileAddr;
    int16_t ObjH, ObjV;
    //uint32_t Sprite[128];
    for(uint32_t OAM_ADDR = OAM_ADDR_BASE + (128*8);OAM_ADDR >= (OAM_ADDR_BASE);OAM_ADDR-=8){
        OBJattr.at0 = MemRead16(OAM_ADDR);
        OBJattr.at1 = MemRead16(OAM_ADDR+0x2);
        OBJattr.at2 = MemRead16(OAM_ADDR+0x4);
        OBJattr.at3 = MemRead16(OAM_ADDR+0x8);
        Priority = (OBJattr.at2 >> 10) & 0x3;
        if(Priority != prio)continue;
        if(OBJattr.at0 == 0 && OBJattr.at1 == 0 && OBJattr.at2 == 0 && OBJattr.at3 == 0)continue;
        RSFlag = ((OBJattr.at0 >> 8) & 0x1);
        if(RSFlag){
            RSparam = (OBJattr.at1 >> 9) & 0x1f;
            Disable = 0;
            affine = LoadOBJAffine(affine, RSparam);
        }
        else{Disable = (OBJattr.at0 >> 9) & 0x1;}
        if(Disable == 1)continue;
        //if(OAM_ADDR == 0x7000038)printf("OAM:%x Content %x\n", OAM_ADDR, OBJattr.at0);
        Xcoord = (OBJattr.at1 & 0x1ff);
        Ycoord = (OBJattr.at0 & 0xff);
        OBJMode = (OBJattr.at0 >> 10) & 0x3;
        if(OBJMode == 2)continue;
        TileNum = (OBJattr.at2 & 0x3ff);
        Mosaic = (OBJattr.at0 >> 12) & 0x1;
        Shape = (OBJattr.at0 >> 14) & 0x3;
        Color = (OBJattr.at0 >> 13) & 0x1;
        if(Color == 0)chr_pal = (OBJattr.at2 >> 12) & 0xf;
        OBJSize = (OBJattr.at1 >> 14) & 0x3;
        DoubleFlag = (OBJattr.at0 >> 9) & 0x1;
        HFlip = (OBJattr.at1 >> 12) & 0x1;
        VFlip = (OBJattr.at1 >> 13) & 0x1;
        TileAddr = 0x6010000 + ((OBJattr.at2 & 0x3ff) << 5);
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
        if(Xcoord >= 0x100)Xcoord = (int16_t)(Xcoord - 0x1FF);
        int16_t x_max = 240, y_max = 160;
        if(DoubleFlag){
            ObjV = ObjV*2;
            ObjH = ObjH*2;
            y_max = (Ycoord + ObjV);
            x_max = (Xcoord + ObjH);
        }
        else{
            y_max = (Ycoord + ObjV);
            x_max = (Xcoord + ObjH);
        }
        //if(y_max < Ycoord)Ycoord -= 256;
        int32_t sprx;
        int32_t spry;
        //OBJwin process enable
        if(OBJMode == 2)objwin = 1;

        if(vcount >= Ycoord && vcount < y_max){
            uint32_t palette[32];
            for(int i=0; i<32; i++){
                if(OBJMode == 1)palette[i] = (ColorFormatTranslate(MemRead16(PALETTE_ADDR + 0x2 * i)) & 0xffffff00) | 0x80;
                else if(OBJMode == 0){palette[i] = ColorFormatTranslate(MemRead16(PALETTE_ADDR + 0x2 * i));}
            }
            if(RSFlag){
                int32_t rcy = Ycoord + (ObjV>>1);
                int32_t rcx = Xcoord + (ObjH>>1);
                int32_t PA, PB, PC, PD;
                PA = affine->PA;
                PB = affine->PB;
                PC = affine->PC;
                PD = affine->PD;
                int16_t tx;
                int16_t ty;
                
                if(DoubleFlag){
                    rcy = Ycoord + (ObjV>>2);
                    rcx = Xcoord + (ObjH>>2);
                    //printf("Xcoord:%d, Ycoord:%d, ObjH:%d, ObjV:%d, x_max:%d, y_max:%d\n", Xcoord, Ycoord, ObjH, ObjV, x_max, y_max);
                    //printf("OAM:%x, Xcoord:%d, Ycoord:%d\n", OAM_ADDR, Xcoord, Ycoord);
                    //getchar();
                    for(sprx = Xcoord;sprx < x_max;sprx++){
                        if(sprx < 240 && sprx >=0){
                            tx = fx_multiply(PA,(sprx - (Xcoord + (ObjH>>1)))) + fx_multiply(PB,(vcount - (Ycoord + (ObjV>>1)))) + rcx;
                            ty = fx_multiply(PC,(sprx - (Xcoord + (ObjH>>1)))) + fx_multiply(PD,(vcount - (Ycoord + (ObjV>>1)))) + rcy;
                            /*if(vcount == Ycoord || vcount == (Ycoord + ObjV - 1) || sprx == Xcoord || sprx == (Xcoord + ObjH -1)){
                                *(uint32_t *)(screen + (vcount)*240*4 + (sprx)*4) = ColorFormatTranslate(0x0);
                            }*/
                            if(tx < (Xcoord + (ObjH>>1)) && ty < (Ycoord + (ObjV>>1)) && tx >= Xcoord && ty >= Ycoord){
                                uint8_t data = MemRead8(TileAddr + 0x400*((ty - Ycoord)/8) + 0x40 * ((tx - Xcoord)/8) + ((ty - Ycoord) % 8) * 8 + ((tx - Xcoord) % 8));
                                //uint8_t data = 0x1;
                                if(data != 0){
                                    if(objwin){
                                        if(*(uint32_t *)(screen + vcount*240*4 + sprx*4) == 0x1)*(uint32_t *)(screen + vcount*240*4 + sprx*4) = palette[data];
                                    }
                                    else{*(uint32_t *)(screen + vcount*240*4 + sprx*4) = palette[data];}
                                }
                            }
                        }
                    }
                }
                else{
                    for(sprx = Xcoord;sprx < x_max;sprx++){
                        if(sprx < 240 && sprx >=0){
                            tx = fx_multiply(PA,(sprx - rcx)) + fx_multiply(PB,(vcount - rcy)) + rcx;
                            ty = fx_multiply(PC,(sprx - rcx)) + fx_multiply(PD,(vcount - rcy)) + rcy;
                            /*if(vcount == Ycoord || vcount == (Ycoord + ObjV - 1) || sprx == Xcoord || sprx == (Xcoord + ObjH -1)){
                                *(uint32_t *)(screen + (vcount)*240*4 + (sprx)*4) = ColorFormatTranslate(0x0);
                            }*/
                            if(tx < (Xcoord + ObjH) && ty < (Ycoord + ObjV) && tx >= Xcoord && ty >= Ycoord){
                                uint8_t data = MemRead8(TileAddr + 0x400 * ((ty - Ycoord)/8) + 0x40 * ((tx - Xcoord)/8) + ((ty - Ycoord) % 8) * 8 + ((tx - Xcoord) % 8));
                                if(data != 0){
                                    if(objwin){
                                        if(*(uint32_t *)(screen + vcount*240*4 + sprx*4) == 0x1)*(uint32_t *)(screen + vcount*240*4 + sprx*4) = palette[data];
                                    }
                                    else{*(uint32_t *)(screen + vcount*240*4 + sprx*4) = palette[data];}
                                }
                            }
                        }
                    }
                }
            }
            else{
                for(sprx = Xcoord;sprx < x_max;sprx++){
                    if((sprx >= 0 && sprx < 240)){
                        /*if(vcount == Ycoord || vcount == (Ycoord + ObjV - 1) || sprx == Xcoord || sprx == (Xcoord + ObjH -1)){
                            *(uint32_t *)(screen + (vcount)*240*4 + (sprx)*4) = ColorFormatTranslate(0x0);
                        }*/
                        uint8_t data = MemRead8(TileAddr + 0x400 * ((vcount - Ycoord) / 8) + 0x40 * ((sprx - Xcoord) / 8) + ((vcount - Ycoord) % 8) * 8 + ((sprx - Xcoord) % 8));
                        if(data != 0){
                            if(objwin){
                                if(*(uint32_t *)(screen + vcount*240*4 + sprx*4) == 0x1)*(uint32_t *)(screen + vcount*240*4 + sprx*4) = palette[data];
                            }
                            else{*(uint32_t *)(screen + vcount*240*4 + sprx*4) = palette[data];}
                        }
                    }
                }
            }
        }
    }
    //x2 = A(x1-x0) + B(y1-y0) + x0
    //y2 = C(x1-x0) + D(y1-y0) + y0
}

void DrawBG(SDL_Renderer* renderer , uint16_t vcount, void *screen){
    struct BG_CNT BG_CNT[4];
    uint16_t DISP = MemRead16(DISPCNT);
    BG_CNT[0] = LoadBGCNT(BG_CNT[0], 0);
    BG_CNT[1] = LoadBGCNT(BG_CNT[1], 1);
    BG_CNT[2] = LoadBGCNT(BG_CNT[2], 2);
    BG_CNT[3] = LoadBGCNT(BG_CNT[3], 3);
    uint8_t Mode = (DISP & 0x7);
    uint8_t Frame = (DISP >> 4) & 0x1;
    uint8_t Blank = (DISP >> 7) & 0x1;
    uint8_t BG_enable[3] = {0xf,0x7,0xc};
    int8_t priority, bg_idx;
    uint8_t affine = 0;
    switch(Mode){
        case 0:
        case 1:
        case 2:
            uint8_t enb = ((DISP >> 8) & 0xf) & BG_enable[Mode];
            
            /*for(priority = 3; priority >= 0; priority--){
                //printf("BG Mode:%u, priority:%u\n", Mode, priority);
                for(bg_idx = 3; bg_idx >= 0; bg_idx--){
                    //if BG_X not enable, passed
                    if(!(enb & (1 << bg_idx)))continue;
                    //check BG_X prority
                    if(BG_CNT[bg_idx].Prio != priority)continue;
                    //check if affine
                    if(Mode == 2 || (Mode == 1 && bg_idx == 2))affine = 1;
                    
                    if(affine){

                    }else{
                        uint16_t offset_y = vcount + MemRead16(0x4000012 + 4*bg_idx);//BG_X Y-Offset
                        uint8_t x;
                        for(x = 0; x < 240; x++){
                            uint16_t offset_x = x + MemRead16(0x4000010 + 4*bg_idx);
                            uint16_t chr_x = offset_x & 7;
                            uint16_t chr_y = offset_y & 7;
                            //uint
                        }
                    }
                }
            }*/
    }
}

void DrawScanLine(uint16_t reg_vcount, SDL_Texture* texture, SDL_Renderer* renderer, void *screen){
    uint16_t DISP = MemRead16(DISPCNT);
    SDL_LockTexture(texture, NULL, &screen, &tex_pitch);
    if((DISP & 7) > 2){
        //DrawBG(renderer);
        //printf("render obj\n");
        DrawSprite(renderer, 0, reg_vcount, screen);
        DrawSprite(renderer, 1, reg_vcount, screen);
        DrawSprite(renderer, 2, reg_vcount, screen);
        DrawSprite(renderer, 3, reg_vcount, screen);
        objwin = 0;
    }
    else{
        //DrawBG(renderer, reg_vcount, screen);
        //printf("render obj\n");
        DrawOBJwindow(renderer, reg_vcount, screen);
        DrawSprite(renderer, 0, reg_vcount, screen);
        DrawSprite(renderer, 1, reg_vcount, screen);
        DrawSprite(renderer, 2, reg_vcount, screen);
        DrawSprite(renderer, 3, reg_vcount, screen);
        objwin = 0;
    }
}

/*void DrawScreen(SDL_Renderer *renderer){
    DrawSprite(renderer);
}*/

void PPU_update(SDL_Texture* texture, SDL_Renderer* renderer, void *screen){
    uint16_t reg_vcount = MemRead8(VCOUNT);
    uint16_t reg_status = MemRead16(DISPSTAT);
    uint8_t reg_hblank = 0;
    uint8_t reg_vblank = 0;
    uint8_t reg_vc_match = 0;
    uint8_t disp_vcount = (reg_status >> 8) & 0xff;
    uint32_t horizon = 0;
    uint32_t vertical = 0;
    if(reg_vcount == disp_vcount && disp_vcount != 0){
        reg_vc_match = 1;
        //printf("v-count match, reg:%d, disp:%d\n", reg_vcount, disp_vcount);
    }
    else{reg_vc_match = 0;}

    if(reg_vcount >= VDRAW && reg_vcount < 228){
        reg_vblank = 1;
        DMA_Transfer(DMA_CH, 1);
    }
    else{reg_vblank = 0;}

    if(horizon > HBLANK_ZERO_CYCLE){
        reg_hblank = 1;
        DMA_Transfer(DMA_CH, 2);
    }
    else{reg_hblank = 0;}

    if(horizon == 0){
        if(reg_vcount == 0){
            uint32_t addr;
            uint32_t addr_e = 160 * 240 * 4;
            SDL_LockTexture(texture, NULL, &screen, &tex_pitch);
            for (addr=0; addr < addr_e; addr += 0x10){
                *(uint32_t *)(screen + (addr | 0x0)) = ColorFormatTranslate(0x7fff);
                *(uint32_t *)(screen + (addr | 0x4)) = ColorFormatTranslate(0x7fff);
                *(uint32_t *)(screen + (addr | 0x8)) = ColorFormatTranslate(0x7fff);
                *(uint32_t *)(screen + (addr | 0xc)) = ColorFormatTranslate(0x7fff);
            }
        }
        if(reg_vcount < VDRAW ){
            DrawScanLine(reg_vcount, texture, renderer, screen);
        }
        reg_vcount += 1;
        reg_status = ((reg_status & 0xfff8) | (reg_vc_match << 2) | (reg_hblank << 1) | (reg_vblank));
        if(reg_vcount == VDRAW)reg_status |= (1 << 3);
        if(reg_vcount == 228){
            SDL_UnlockTexture(texture);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            reg_vcount = 0;
        }
        if((reg_status & (1 << 3))){
            PPUMemWrite16(0x4000202, 0x1);
            //reg_status &= 0xfff7;
        }
        else if((reg_status & (1 << 4))){
            PPUMemWrite16(0x4000202, 0x1);
        }
        PPUMemWrite16(DISPSTAT, reg_status);
        PPUMemWrite16(VCOUNT, reg_vcount);
    }
}