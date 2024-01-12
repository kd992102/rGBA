#include "timer.h"
#include "io.h"

static const uint8_t shift_lut[4] = {0, 6 ,8, 10};

uint16_t * ReadTimer(uint16_t *tmcnt){
    tmcnt[0] = MemRead16(TM0CNT);
    tmcnt[1] = MemRead16(TM1CNT);
    tmcnt[2] = MemRead16(TM2CNT);
    tmcnt[3] = MemRead16(TM3CNT);
    return tmcnt;
}

void Timer_Clock(uint64_t cycle){
    uint8_t idx;
    uint8_t overflow = 0;
    uint16_t count, reload;
    TM_CNT = ReadTimer(TM_CNT);
    for(idx = 0; idx < 4; idx++){
        if(!(TM_CNT[idx] & TMR_ENB)){
            overflow = 1;
            continue;
        }

        if(TMCNT[idx] & TMR_CASCADE){
            if(overflow){
                switch(idx){
                    case 0:
                        count = MemRead16(TM0_D);
                        MemWrite16(TM0_D, count+1);
                        break;
                    case 1:
                        count = MemRead16(TM0_D);
                        MemWrite16(TM1_D, count+1);
                        break;
                    case 2:
                        count = MemRead16(TM0_D);
                        MemWrite16(TM2_D, count+1);
                        break;
                    case 3:
                        count = MemRead16(TM0_D);
                        MemWrite16(TM3_D, count+1);
                        break;
                }
            }
            else{
                uint8_t shift = shift_lut[TM_CNT[idx] & 3];
                uint64_t inc = (TM_CNT[idx] += cycles) >> shift;

                
            }
        }

        if(overflow = (count > 0xffff)){
            count = reload + count - 0x10000;
            if()
        }
    }
}