#include <stdint.h>
#define DMA0SAD 0x40000B0
#define DMA1SAD 0x40000BC
#define DMA2SAD 0x40000C8
#define DMA3SAD 0x40000D4
#define DMA0DAD 0x40000B4
#define DMA1DAD 0x40000C0
#define DMA2DAD 0x40000CC
#define DMA3DAD 0x40000D8
#define DMA0CNT 0x40000B8
#define DMA1CNT 0x40000C4
#define DMA2CNT 0x40000D0
#define DMA3CNT 0x40000DC

#define DMA_ENABLE (1 << 15)
#define DMA_INTERRUPT_ENABLE (1 << 30)
#define DMA_TIMEING_IMMEDIATE 0
#define DMA_TIMEING_VBLANK (1 << 27)
#define DMA_TIMEING_HBLANK (1 << 28)
#define DMA_16 0
#define DMA_32 (1 << 26)
#define DMA_REPEAT (1 << 25)
#define DMA_SOURCE_INC 0
#define DMA_SOURCE_DEC (1 << 23)
#define DMA_SOURCE_FIX (1 << 24)
#define DMA_DEST_INC 0
#define DMA_DEST_DEC (1 << 21)
#define DMA_DEST_FIX (1 << 22)
#define DMA_DEST_RELOAD (0x3 << 21)
#define DMA_16NOW (DMA_ENABLE | DMA_TIMEING_IMMEDIATE |DMA_16)
#define DMA_32NOW (DMA_ENABLE | DMA_TIMEING_IMMEDIATE |DMA_32)

//For CNT_H bit 0xC-0xD
enum DMA_TIME_MODE{
    Immediate, VBlank, Hblank, Special
};

struct CHANNEL{
    uint32_t DMASAD;
    uint32_t DMADAD;
    uint32_t DMACNT;
};

typedef struct CHANNEL DMA_CHANNEL;

struct DMA_CHANNEL_ARRAY{
    DMA_CHANNEL DMAX[4];
};

typedef struct DMA_CHANNEL_ARRAY DMA;

void DMA_Transfer(DMA DMA_CH, uint8_t TimeMode);
DMA DMA_CHANNEL_READ(DMA DMA_CH);