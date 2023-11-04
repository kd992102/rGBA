#include <stdint.h>
//uint32_t FileSize(FILE *ptr);
typedef enum InterruptMask IntMask;

enum InterruptMask {
    LCD_VBLANK = (1<<0);
    LCD_HBLANK = (1<<1);
    LCD_VCOUNT = (1<<2);
    TIMER0OF = (1<<3);
    TIMER1OF = (1<<4);
    TIMER2OF = (1<<5);
    TIMER3OF = (1<<6);
    SERIAL = (1<<7);
    DMA0 = (1<<8);
    DMA1 = (1<<9);
    DMA2 = (1<<10);
    DMA3 = (1<<11);
    KEYPAD = (1<<12);
    GAMEPAK = (1<<13);
};

void InitCpu(uint32_t BaseAddr);
//Exeception
void Reset();
void IRQhandler();