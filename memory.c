#include "memory.h"
#include <stdio.h>

void Init_GbaMem(Gba_Memory *Mem){
    Mem->GIMem = malloc(sizeof(GI_Memory));
    Mem->GIMem->BIOS = malloc(sizeof(uint8_t) * BIOS_MEM_SIZE);
    Mem->GIMem->WRAM_board = malloc(sizeof(uint8_t) * WRAM_BOARD_MEM_SIZE);
    Mem->GIMem->WRAM_chip = malloc(sizeof(uint8_t) * WRAM_CHIP_MEM_SIZE);
    Mem->GIMem->IO_Reg = malloc(sizeof(uint8_t) * IO_REG_MEM_SIZE);
    Mem->DisMem = malloc(sizeof(Dis_Memory));
    Mem->DisMem->Palette_RAM = malloc(sizeof(uint8_t) * PALETTE_RAM_MEM_SIZE);
    Mem->DisMem->VRAM = malloc(sizeof(uint8_t) * VRAM_MEM_SIZE);
    Mem->DisMem->OAM = malloc(sizeof(uint8_t) * OAM_MEM_SIZE);
    Mem->ExMem = malloc(sizeof(Ex_Memory));
    Mem->ExMem->Game_ROM1 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    Mem->ExMem->Game_SRAM = malloc(sizeof(uint8_t) * GAME_SRAM_MEM_SIZE);
}

void Release_GbaMem(Gba_Memory *Mem){
    free(Mem);
    Mem = NULL;
}

uint32_t MemoryAddrReloc(Gba_Memory *Mem, uint32_t addr)
{
    //printf("%08x\n", addr);
    switch(addr & 0xf000000){
        case BIOS_ADDR_BASE:
            if(addr ^ BIOS_ADDR_BASE < BIOS_MEM_SIZE)return (uint32_t)(Mem->GIMem->BIOS) + (uint32_t)(addr - BIOS_ADDR_BASE);
        case WRAMB_ADDR_BASE:
            if(addr ^ WRAMB_ADDR_BASE < WRAM_BOARD_MEM_SIZE)return (uint32_t)(Mem->GIMem->WRAM_board) + (uint32_t)(addr - WRAM_BOARD_MEM_SIZE);
        case WRAMC_ADDR_BASE:
            if(addr ^ WRAMC_ADDR_BASE < WRAM_CHIP_MEM_SIZE)return (uint32_t)(Mem->GIMem->WRAM_chip) + (uint32_t)(addr - WRAM_CHIP_MEM_SIZE);
        case IOREG_ADDR_BASE:
            if(addr ^ IOREG_ADDR_BASE < IO_REG_MEM_SIZE)return (uint32_t)(Mem->GIMem->IO_Reg) + (uint32_t)(addr - IOREG_ADDR_BASE);
        case PALETTE_RAM_ADDR_BASE:
            if(addr ^ PALETTE_RAM_ADDR_BASE < PALETTE_RAM_MEM_SIZE)return (uint32_t)(Mem->DisMem->Palette_RAM) + (uint32_t)(addr - PALETTE_RAM_ADDR_BASE);
        case VRAM_ADDR_BASE:
            if(addr ^ VRAM_ADDR_BASE < VRAM_MEM_SIZE)return (uint32_t)(Mem->DisMem->VRAM) + (uint32_t)(addr - VRAM_ADDR_BASE);
        case OAM_ADDR_BASE:
            if(addr ^ OAM_ADDR_BASE < OAM_MEM_SIZE)return (uint32_t)(Mem->DisMem->OAM) + (uint32_t)(addr - OAM_ADDR_BASE);
        case GAMEROM_ADDR_BASE:
            if(addr ^ GAMEROM_ADDR_BASE < 0x1000000)return (uint32_t)(Mem->ExMem->Game_ROM1) + (uint32_t)(addr - GAMEROM_ADDR_BASE);
        case 0x9000000:
            if(addr ^ 0x9000000 < 0x1000000)return (uint32_t)(Mem->ExMem->Game_ROM1) + (uint32_t)(addr - GAMEROM_ADDR_BASE);
        case GAME_SRAM_MEM_SIZE:
            if(addr ^ GAMEROM_SDRAM_BASE < GAME_SRAM_MEM_SIZE)return (uint32_t)(Mem->ExMem->Game_SRAM) + (uint32_t)(addr - GAMEROM_SDRAM_BASE);
        default:
            printf("Memory Address %08x Not Used", addr);
            return 1;
    }
}

