#include <stdio.h>
#include "io.h"
#include "timer.h"
#include "sound.h"

static const uint8_t shift_lut[4] = {0, 6 ,8, 10};

void Timer_Clock(uint32_t cycle){
    uint8_t idx;
    uint8_t overflow = 0;
    uint16_t count, reload;
    uint16_t TM_CNT[4];
    uint16_t TM_COUNT[4];
    uint32_t tmr_increcnt[4];
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
        if(idx != 0)printf("i:%d, cnt:%d\n", idx, TM_CNT[idx]);

        if(TM_CNT[idx] & TMR_CASCADE && idx != 0){
            printf("timers cascade\n");
            if(overflow){
                TM_COUNT[idx] = MemRead16(TM0_D + 0x4*idx);
                MemWrite16(TM0_D + 0x4*idx, count+1);
                TM_COUNT[idx] = MemRead16(TM0_D + 0x4*idx);
            }
        }
        else{
            uint8_t shift = shift_lut[TM_CNT[idx] & 3];
            uint64_t inc = (tmr_increcnt[idx] += cycle) >> shift;

            TM_CNT[idx] += inc;
            tmr_increcnt[idx] -= inc << shift;
        }

        if(overflow = (TM_COUNT[idx] > 0xffff)){
            TM_COUNT[idx] = reload + TM_COUNT[idx] - 0x10000;
            uint16_t sound_cnt = MemRead16(SOUNDCNT_H);
            if(((sound_cnt >> 10) & 1) == idx){
                //DMA sound A FIFO
            }
            if(((sound_cnt >> 14) & 1) == idx){
                //DMA sound B FIFO
            }
        }

        if((TM_CNT[idx] & TMR_IRQ) && overflow){
            //trigger irq
        }
    }
}