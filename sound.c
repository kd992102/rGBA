#include "sound.h"
#include "io.h"

uint8_t fifo_a_len;
uint8_t fifo_b_len;
uint8_t fifo_a_samp;
uint8_t fifo_b_samp;
int8_t fifo_a[0x20];
int8_t fifo_b[0x20];

void fifo_a_load(){
    if(fifo_a_len){
        fifo_a_samp = fifo_a[0];
        fifo_a_len--;
        
        for(uint8_t i=0; i<fifo_a_len;i++){
            fifo_a[i] = fifo_a[i+1];
        }
    }
}

void fifo_b_load(){
    if(fifo_b_len){
        fifo_b_samp = fifo_b[0];
        fifo_b_len--;
        
        for(uint8_t i=0; i<fifo_b_len;i++){
            fifo_b[i] = fifo_b[i+1];
        }
    }
}

void fifo_a_copy(){
    if(fifo_a_len + 4 > 0x20){
        fifo_a[fifo_a_len++] = MemRead8(0x40000a0);
        fifo_a[fifo_a_len++] = MemRead8(0x40000a1);
        fifo_a[fifo_a_len++] = MemRead8(0x40000a2);
        fifo_a[fifo_a_len++] = MemRead8(0x40000a3);
    }
}

void fifo_b_copy(){
    if(fifo_b_len + 4 > 0x20){
        fifo_b[fifo_b_len++] = MemRead8(0x40000a4);
        fifo_b[fifo_b_len++] = MemRead8(0x40000a5);
        fifo_b[fifo_b_len++] = MemRead8(0x40000a6);
        fifo_b[fifo_b_len++] = MemRead8(0x40000a7);
    }
}