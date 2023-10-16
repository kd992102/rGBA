#include <stdio.h>
#include "memory.h"
#include "io.h"
#include "dma.h"

extern GbaMem *Mem;

DMA DMA_CHANNEL_READ(DMA DMA_CH){
    DMA_CH.DMAX[0].DMACNT = MemRead32(DMA0CNT);
    DMA_CH.DMAX[0].DMASAD = MemRead32(DMA0SAD);
    DMA_CH.DMAX[0].DMADAD = MemRead32(DMA0DAD);
    DMA_CH.DMAX[1].DMACNT = MemRead32(DMA1CNT);
    DMA_CH.DMAX[1].DMASAD = MemRead32(DMA1SAD);
    DMA_CH.DMAX[1].DMADAD = MemRead32(DMA1DAD);
    DMA_CH.DMAX[2].DMACNT = MemRead32(DMA2CNT);
    DMA_CH.DMAX[2].DMASAD = MemRead32(DMA2SAD);
    DMA_CH.DMAX[2].DMADAD = MemRead32(DMA2DAD);
    DMA_CH.DMAX[3].DMACNT = MemRead32(DMA3CNT);
    DMA_CH.DMAX[3].DMASAD = MemRead32(DMA3SAD);
    DMA_CH.DMAX[3].DMADAD = MemRead32(DMA3DAD);
    return DMA_CH;
}

void DMA_Transfer(DMA DMA_CH, uint8_t TimeMode){
    uint32_t count;
    uint8_t offset;
    uint8_t Reload = 0;
    uint32_t DstAddr;
    uint32_t SrcAddr;
    uint8_t idx;
    DMA_CH.DMAX[0].DMACNT = MemRead32(DMA0CNT);
    DMA_CH.DMAX[0].DMASAD = MemRead32(DMA0SAD);
    DMA_CH.DMAX[0].DMADAD = MemRead32(DMA0DAD);
    DMA_CH.DMAX[1].DMACNT = MemRead32(DMA1CNT);
    DMA_CH.DMAX[1].DMASAD = MemRead32(DMA1SAD);
    DMA_CH.DMAX[1].DMADAD = MemRead32(DMA1DAD);
    DMA_CH.DMAX[2].DMACNT = MemRead32(DMA2CNT);
    DMA_CH.DMAX[2].DMASAD = MemRead32(DMA2SAD);
    DMA_CH.DMAX[2].DMADAD = MemRead32(DMA2DAD);
    DMA_CH.DMAX[3].DMACNT = MemRead32(DMA3CNT);
    DMA_CH.DMAX[3].DMASAD = MemRead32(DMA3SAD);
    DMA_CH.DMAX[3].DMADAD = MemRead32(DMA3DAD);
    for(idx = 0;idx < 4;idx++){
        if(!(DMA_CH.DMAX[idx].DMACNT & (DMA_ENABLE << 16)) || (((DMA_CH.DMAX[idx].DMACNT >> 12) & 0x3) != TimeMode))continue;
        printf("DMA%d Transfer\n", idx);
        count = (DMA_CH.DMAX[idx].DMACNT & 0xffff);
        DstAddr = (DMA_CH.DMAX[idx].DMADAD);
        SrcAddr = (DMA_CH.DMAX[idx].DMASAD);
        if(count == 0){
            if(idx == 3)count = 0x10000;
            else{count = 0x4000;}
        }

        if((DMA_CH.DMAX[idx].DMACNT & DMA_32))offset = 4;
        else{offset = 2;}

        while(count--){
            if(offset == 4)MemWrite32(DMA_CH.DMAX[idx].DMADAD, MemRead32(DMA_CH.DMAX[idx].DMASAD));
            else{MemWrite32(DMA_CH.DMAX[idx].DMADAD, MemRead32(DMA_CH.DMAX[idx].DMASAD));}
            switch(((DMA_CH.DMAX[idx].DMACNT >> 21) & 0x3)){
                case 0:
                    DstAddr += offset;
                    break;
                case 1:
                    DstAddr -= offset;
                    break;
                case 3:
                    Reload = 1;
                    DstAddr += offset;
                    break;
            }
        }
        //lack IRQ
        if(((DMA_CH.DMAX[idx].DMACNT >> 25) & 0x1)){
            count = (DMA_CH.DMAX[idx].DMACNT & 0xffff);
            if(!Reload){
                DMA_CH.DMAX[idx].DMACNT = 0;
                switch(idx){
                    case 0:
                        MemWrite32(DMA0DAD, DMA_CH.DMAX[idx].DMACNT);
                        break;
                    case 1:
                        MemWrite32(DMA1DAD, DMA_CH.DMAX[idx].DMACNT);
                        break;
                    case 2:
                        MemWrite32(DMA1DAD, DMA_CH.DMAX[idx].DMACNT);
                        break;
                    case 3:
                        MemWrite32(DMA1DAD, DMA_CH.DMAX[idx].DMACNT);
                        break;
                }
            }
            continue;
        }

        DMA_CH.DMAX[idx].DMACNT &= ~DMA_ENABLE;
    }
}