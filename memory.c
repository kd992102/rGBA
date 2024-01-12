#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

extern GbaMem *Mem;

void Init_GbaMem(){
    Mem->BIOS = malloc(sizeof(uint8_t) * BIOS_MEM_SIZE);
    memset(Mem->BIOS, 0, BIOS_MEM_SIZE);
    Mem->WRAM_board = malloc(sizeof(uint8_t) * WRAM_BOARD_MEM_SIZE);
    memset(Mem->WRAM_board, 0, WRAM_BOARD_MEM_SIZE);
    Mem->WRAM_chip = malloc(sizeof(uint8_t) * WRAM_CHIP_MEM_SIZE);
    memset(Mem->WRAM_chip, 0, WRAM_CHIP_MEM_SIZE);
    Mem->IO_Reg = malloc(sizeof(uint8_t) * IO_REG_MEM_SIZE);
    memset(Mem->IO_Reg, 0, IO_REG_MEM_SIZE);
    Mem->Palette_RAM = malloc(sizeof(uint8_t) * PALETTE_RAM_MEM_SIZE);
    memset(Mem->Palette_RAM, 0, PALETTE_RAM_MEM_SIZE);
    Mem->VRAM = malloc(sizeof(uint8_t) * VRAM_MEM_SIZE);
    memset(Mem->VRAM, 0, VRAM_MEM_SIZE);
    Mem->OAM = malloc(sizeof(uint8_t) * OAM_MEM_SIZE);
    memset(Mem->OAM, 0, OAM_MEM_SIZE);
    Mem->Game_ROM1 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    memset(Mem->Game_ROM1, 0, GAME_ROM_MAXSIZE);
    Mem->Game_ROM2 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    memset(Mem->Game_ROM2, 0, GAME_ROM_MAXSIZE);
    Mem->Game_ROM3 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    memset(Mem->Game_ROM3, 0, GAME_ROM_MAXSIZE);
    Mem->Game_SRAM = malloc(sizeof(uint8_t) * GAME_SRAM_MEM_SIZE);
    memset(Mem->Game_SRAM, 0xff, GAME_SRAM_MEM_SIZE);
}

void Release_GbaMem(){
    free(Mem);
    Mem = NULL;
}