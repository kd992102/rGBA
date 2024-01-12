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

#define PALETTE_ADDR 0x5000200
typedef int32_t s32;
typedef volatile s32 fixed32;
typedef int16_t s16;
typedef volatile s16 fixed16;

#define FIX_SHIFT 8
#define FIX_SCALE 256
#define FIX_SCALEF 256.0f

static inline fixed32 fx_from_int(int d) {
    return d << FIX_SHIFT;
}

static inline fixed32 fx_from_float(float f) {
    return (fixed32)(f*FIX_SCALE);
}

static inline int fx_to_int(fixed32 fx) {
    return fx / FIX_SCALE;
}

static inline float fx_to_float(fixed32 fx) {
    return fx / FIX_SCALEF;
}

static inline fixed32 fx_multiply(fixed32 fa, fixed32 fb) {
    return (fa*fb)>>FIX_SHIFT;
}

static inline fixed32 fx_division(fixed32 fa, fixed32 fb) {
    return ((fa)*FIX_SCALE)/(fb);
}

struct OAM {
    uint16_t at0;
    uint16_t at1;
    uint16_t at2;
    uint16_t at3;
};

struct OBJ_Affine {
    fixed16 PA;
    fixed16 PB;
    fixed16 PC;
    fixed16 PD;
};

struct BG_CNT {
    uint8_t Prio;
    uint8_t CharBaseBlock;
    uint8_t Mosaic;
    uint8_t Color;
    uint8_t ScreenBaseBlock;
    uint8_t DispAreaOverflow;
    uint8_t ScreenSize;
};

typedef struct OAM OAM_attr;

struct BG_CNT LoadBGCNT(struct BG_CNT BGCNT, uint8_t BGNum);
struct OBJ_Affine * LoadOBJAffine(struct OBJ_Affine * affine, uint16_t GroupNum);
uint32_t ColorFormatTranslate(uint16_t BGR555);

void DrawBG(SDL_Renderer *renderer);
void ScalePixel(void *coord, uint8_t scale, uint8_t ox, uint8_t oy);
void DrawSprite(SDL_Renderer* renderer, uint8_t prio, uint16_t vcount, void *screen);
void DrawScanLine(uint16_t reg_vcount, SDL_Texture* texture, SDL_Renderer* renderer, void *screen);
void PPUInit(SDL_Renderer* renderer, SDL_Window* window, SDL_Texture* texture);
void PPU_update(uint32_t cycle, SDL_Texture* texture, SDL_Renderer* renderer, void *screen);
