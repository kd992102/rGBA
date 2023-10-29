#include <stdint.h>
#include <SDL2/SDL.h>
//LCD I/O BG Scrolling
#define DISPCNT 0x4000000
#define DISPSTAT 0x4000004
#define VCOUNT 0x4000006
#define BG0CNT 0x4000008
#define BG1CNT 0x400000A
#define BG2CNT 0x400000C
#define BG3CNT 0x400000E
#define BG0HOFS 0x4000010
#define BG0VOFS 0x4000012
#define BG1HOFS 0x4000014
#define BG1VOFS 0x4000016
#define BG2HOFS 0x4000018
#define BG2VOFS 0x400001a
#define BG3HOFS 0x400001c
#define BG3VOFS 0x400001e

#define V_BLANK_FLAG 1
#define H_BLANK_FLAG (1 << 1)
#define V_COUNT_FLAG (1 << 2)

#define VDRAW 160
#define HDRAW 240
#define HDRAW_CYCLE 960 //240*4
#define HBLANK_ZERO_CYCLE 1006
#define HBLANK_CYCLE 272 //68*4
#define SCANLINE_CYCLE (HDRAW_CYCLE + HBLANK_CYCLE) //1232
#define VDRAW_CYCLE (VDRAW * SCANLINE_CYCLE)
#define VBLANK_CYCLE (68 * SCANLINE_CYCLE)

//LCD I/O BG Rotation/Scaling
#define BG2X_L 0x4000028
#define BG2X_H 0x400002A
#define BG2Y_L 0x400002C
#define BG2Y_H 0x400002E
#define BG2PA 0x4000020
#define BG2PB 0x4000022
#define BG2PC 0x4000024
#define BG2PD 0x4000026
#define BG3X_L 0x4000038
#define BG3X_H 0x400003A
#define BG3Y_L 0x400003C
#define BG3Y_H 0x400003E
#define BG3PA 0x4000030
#define BG3PB 0x4000032
#define BG3PC 0x4000034
#define BG3PD 0x4000036

//LCD I/O Window Feature
#define WIN0H 0x4000040
#define WIN1H 0x4000042
#define WIN0V 0x4000044
#define WIN1V 0x4000046
#define WININ 0x4000048
#define WINOUT 0x400004A

//LCD I/O Mosaic Function
#define MOSAIC 0x400004C

//LCD I/O Color Special Effects
#define BLDCNT 0x4000050
#define BLDALPHA 0x4000052
#define BLDY 0x4000054

struct OAM {
    uint16_t at0;
    uint16_t at1;
    uint16_t at2;
    uint16_t at3;
};

typedef struct OAM OAM_attr;

void DrawSprite(SDL_Renderer* renderer, uint16_t vcount, uint16_t h);
void PPUInit(SDL_Renderer* renderer, SDL_Window* window, SDL_Texture* texture);
void DrawScanLine(uint16_t vcount, SDL_Texture* texture, SDL_Renderer* renderer);
void PPU_update(uint32_t cycle, SDL_Texture* texture, SDL_Renderer* renderer);
