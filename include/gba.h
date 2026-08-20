#ifndef GBA_H
#define GBA_H

#include <stdint.h>
#include "arm7tdmi.h"
#include "memory.h"

//uint32_t FileSize(FILE *ptr);
typedef enum InterruptMask IntMask;

typedef enum IRQ_VECTOR IrqVec;

enum IRQ_VECTOR {
    LCD_VBLANK = (1<<0),
    LCD_HBLANK = (1<<1),
    LCD_VCOUNT = (1<<2),
    TIMER0OF = (1<<3),
    TIMER1OF = (1<<4),
    TIMER2OF = (1<<5),
    TIMER3OF = (1<<6),
    SERIAL = (1<<7),
    DMA0 = (1<<8),
    DMA1 = (1<<9),
    DMA2 = (1<<10),
    DMA3 = (1<<11),
    KEYPAD = (1<<12),
    GAMEPAK = (1<<13)
};

typedef struct GBA_Context {
    Gba_Cpu cpu;
    GbaMem mem;
    // 未來還會加入 PPU, APU, Timer 等
    uint64_t master_clock; 
} GBA_Context;

void GBA_Init(GBA_Context *ctx);
void GBA_Step(GBA_Context *ctx); // 執行一個 CPU 指令並推進時鐘

#endif