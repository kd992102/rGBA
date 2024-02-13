#include "io.h"
#include "timer.h"
#include "sound.h"

static const uint8_t shift_lut[4] = {0, 6 ,8, 10};

void Timer_Clock(uint32_t cycle){
    uint8_t idx;
    uint8_t overflow = 0;
    uint16_t count, reload;
    uint16_t TM_CNT[4];
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

        if(TM_CNT[idx] & TMR_CASCADE){
            if(overflow){
                switch(idx){
                    case 0:
                        count = MemRead16(TM0_D);
                        MemWrite16(TM0_D, count+1);
                        count = MemRead16(TM0_D);
                        break;
                    case 1:
                        count = MemRead16(TM1_D);
                        MemWrite16(TM1_D, count+1);
                        count = MemRead16(TM1_D);
                        break;
                    case 2:
                        count = MemRead16(TM2_D);
                        MemWrite16(TM2_D, count+1);
                        count = MemRead16(TM2_D);
                        break;
                    case 3:
                        count = MemRead16(TM3_D);
                        MemWrite16(TM3_D, count+1);
                        count = MemRead16(TM3_D);
                        break;
                }
            }
            else{
                uint8_t shift = shift_lut[TM_CNT[idx] & 3];
                uint64_t inc = (tmr_increcnt[idx] += cycle) >> shift;

                TM_CNT[idx] += inc;
                tmr_increcnt[idx] -= inc << shift;
            }
        }

        if(overflow = (count > 0xffff)){
            count = reload + count - 0x10000;
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