#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
// #include "dma.h"
#include "gba.h"
// #include "timer.h"
// #include "ppu.h"

uint32_t bios_size;
uint32_t rom_size;
//static const char bios_file_name[] = "gba_bios.bin";
//static const char rom_file_name[] = "tetris.gba";

int main(int argc, char *argv[]){
    GBA_Context gba_ctx;
    GBA_Init(&gba_ctx);

    // Main loop would be:
    // while (1) {
    //     GBA_Step(&gba_ctx);
    // }
    
    return 0;
}