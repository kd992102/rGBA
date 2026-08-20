#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

void Memory_Init(GbaMem *mem){
    memset(mem->bios, 0, BIOS_SIZE);
    memset(mem->ewram, 0, WRAM_BOARD_SIZE);
    memset(mem->iwram, 0, WRAM_CHIP_SIZE);
    memset(mem->reserved, 0, RESERVED_SIZE);
    memset(mem->io, 0, IO_REG_SIZE);
    memset(mem->palatte, 0, PALETTE_RAM_SIZE);
    memset(mem->vram, 0, VRAM_SIZE);
    memset(mem->oam, 0, OAM_SIZE);
    memset(mem->sram, 0xff, GAME_SRAM_SIZE);
    mem->rom = NULL;
    mem->rom_size = 0;
    mem->rom_mask = 0;
}

void Memory_Reset(GbaMem *mem){
    Memory_Init(mem);
}

void Memory_Release(GbaMem *mem){
    if(mem->rom){
        free(mem->rom);
        mem->rom = NULL;
    }
}
