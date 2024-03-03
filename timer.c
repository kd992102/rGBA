#include <stdio.h>
#include "io.h"
#include "timer.h"
#include "sound.h"
#include "dma.h"

extern DMA DMA_CH;
extern uint8_t fifo_a_len;
extern uint8_t fifo_b_len;

static const uint8_t shift_lut[4] = {0, 6 ,8, 10};
uint32_t tmr_increcnt[4];

void Timer_Clock(uint32_t cycle){
    uint8_t idx;
    uint8_t overflow = 0;
    uint32_t count, reload = 0;
    uint16_t TM_CNT[4];
    uint16_t TM_COUNT[4];
    //uint32_t tmr_increcnt[4];
    TM_CNT[0] = MemRead16(TM0CNT);
    TM_CNT[1] = MemRead16(TM1CNT);
    TM_CNT[2] = MemRead16(TM2CNT);
    TM_CNT[3] = MemRead16(TM3CNT);

    for(idx = 0; idx < 4; idx++){
        //If TM_REG Not enable, continue
        if(!(TM_CNT[idx] & TMR_ENB)){
            overflow = 0;
            continue;
        }
        //printf("Timer Enable\n");

        if(TM_CNT[idx] & TMR_CASCADE){
            //printf("timers cascade\n");
            if(overflow){
                TM_COUNT[idx] = MemRead16(TM0_D + 0x4*idx);
                MemWrite16(TM0_D + 0x4*idx, TM_COUNT[idx]+1);
                TM_COUNT[idx] = MemRead16(TM0_D + 0x4*idx);
            }
        }
        else{
            //printf("Timer count %u\n", TM_COUNT[idx]);
            uint8_t shift = shift_lut[TM_CNT[idx] & 3];
            uint64_t inc = (tmr_increcnt[idx] += cycle) >> shift;

            TM_COUNT[idx] += inc;
            tmr_increcnt[idx] -= (inc << shift);
        }
        
        if((overflow = (TM_COUNT[idx] > 0xfffe))){
            //printf("Timer Overflow\n");
            TM_COUNT[idx] = reload + TM_COUNT[idx] - 0x10000;
            uint16_t sound_cnt = MemRead16(SOUNDCNT_H);
            if(((sound_cnt >> 10) & 1) == idx){
                //DMA sound A FIFO
                fifo_a_load();
                if(fifo_a_len <= 0x10)DMA_Transfer_fifo(DMA_CH, 1);
            }
            if(((sound_cnt >> 14) & 1) == idx){
                //DMA sound B FIFO
                fifo_b_load();
                if(fifo_b_len <= 0x10)DMA_Transfer_fifo(DMA_CH, 2);
            }
        }

        if((TM_CNT[idx] & TMR_IRQ) && overflow){
            //trigger irq
        }
    }
}