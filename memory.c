#include "memory.h"
#include <stdio.h>
#include <string.h>

void Init_GbaMem(Gba_Memory *Mem){
    Mem->GIMem = malloc(sizeof(GI_Memory));
    Mem->GIMem->BIOS = malloc(sizeof(uint8_t) * BIOS_MEM_SIZE);
    memset(Mem->GIMem->BIOS, 0, BIOS_MEM_SIZE);
    Mem->GIMem->WRAM_board = malloc(sizeof(uint8_t) * WRAM_BOARD_MEM_SIZE);
    memset(Mem->GIMem->WRAM_board, 0, WRAM_BOARD_MEM_SIZE);
    Mem->GIMem->WRAM_chip = malloc(sizeof(uint8_t) * WRAM_CHIP_MEM_SIZE);
    memset(Mem->GIMem->WRAM_chip, 0, WRAM_CHIP_MEM_SIZE);
    //Mem->GIMem->Reserved = malloc(sizeof(uint8_t) * RESERVED_MEM_SIZE);
    //memset(Mem->GIMem->Reserved, 0, RESERVED_MEM_SIZE);
    Mem->GIMem->IO_Reg = malloc(sizeof(uint8_t) * IO_REG_MEM_SIZE);
    memset(Mem->GIMem->IO_Reg, 0, IO_REG_MEM_SIZE);
    Mem->DisMem = malloc(sizeof(Dis_Memory));
    Mem->DisMem->Palette_RAM = malloc(sizeof(uint8_t) * PALETTE_RAM_MEM_SIZE);
    memset(Mem->DisMem->Palette_RAM, 0, PALETTE_RAM_MEM_SIZE);
    Mem->DisMem->VRAM = malloc(sizeof(uint8_t) * VRAM_MEM_SIZE);
    memset(Mem->DisMem->VRAM, 0, VRAM_MEM_SIZE);
    Mem->DisMem->OAM = malloc(sizeof(uint8_t) * OAM_MEM_SIZE);
    memset(Mem->DisMem->OAM, 0, OAM_MEM_SIZE);
    Mem->ExMem = malloc(sizeof(Ex_Memory));
    Mem->ExMem->Game_ROM1 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    memset(Mem->ExMem->Game_ROM1, 0, GAME_ROM_MAXSIZE);
    Mem->ExMem->Game_ROM2 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    memset(Mem->ExMem->Game_ROM2, 0, GAME_ROM_MAXSIZE);
    Mem->ExMem->Game_ROM3 = malloc(sizeof(uint8_t) * GAME_ROM_MAXSIZE);
    memset(Mem->ExMem->Game_ROM3, 0, GAME_ROM_MAXSIZE);
    Mem->ExMem->Game_SRAM = malloc(sizeof(uint8_t) * GAME_SRAM_MEM_SIZE);
    memset(Mem->ExMem->Game_SRAM, 0, GAME_SRAM_MEM_SIZE);
}

void Release_GbaMem(Gba_Memory *Mem){
    free(Mem);
    Mem = NULL;
}

uint32_t MemoryAddrReloc(Gba_Memory *Mem, uint32_t addr)
{
    //if(addr < 0x8000000)printf("addr %08x\n", addr);
    switch(addr & 0xf000000){
        case BIOS_ADDR_BASE:
            if((addr ^ BIOS_ADDR_BASE) < BIOS_MEM_SIZE)return (uint32_t)(Mem->GIMem->BIOS) + (uint32_t)(addr - BIOS_ADDR_BASE);
            break;
        case WRAMB_ADDR_BASE:
            if((addr ^ WRAMB_ADDR_BASE) < WRAM_BOARD_MEM_SIZE)return (uint32_t)(Mem->GIMem->WRAM_board) + (uint32_t)(addr -WRAMB_ADDR_BASE);
            if(addr >= (WRAMB_ADDR_BASE + WRAM_BOARD_MEM_SIZE))return (uint32_t)(Mem->GIMem->WRAM_board) + (uint32_t)(((addr - WRAMC_ADDR_BASE) % WRAM_BOARD_MEM_SIZE));
            break;
        case WRAMC_ADDR_BASE:
            if((addr ^ WRAMC_ADDR_BASE) < WRAM_CHIP_MEM_SIZE)return (uint32_t)(Mem->GIMem->WRAM_chip) + (uint32_t)(addr - WRAMC_ADDR_BASE);
            if(addr >= (WRAMC_ADDR_BASE + WRAM_CHIP_MEM_SIZE))return (uint32_t)(Mem->GIMem->WRAM_chip) + (uint32_t)(((addr - WRAMC_ADDR_BASE) % WRAM_CHIP_MEM_SIZE));
            break;
        case IOREG_ADDR_BASE:
            if((addr ^ IOREG_ADDR_BASE) < IO_REG_MEM_SIZE)return (uint32_t)(Mem->GIMem->IO_Reg) + (uint32_t)(addr - IOREG_ADDR_BASE);
            if(addr >= (IOREG_ADDR_BASE + 0x3FF))return (uint32_t)(Mem->GIMem->IO_Reg) + (uint32_t)(((addr - IOREG_ADDR_BASE) % 0x400));
            break;
        case PALETTE_RAM_ADDR_BASE:
            if((addr ^ PALETTE_RAM_ADDR_BASE) < PALETTE_RAM_MEM_SIZE)return (uint32_t)(Mem->DisMem->Palette_RAM) + (uint32_t)(addr - PALETTE_RAM_ADDR_BASE);
            if(addr >= (PALETTE_RAM_ADDR_BASE + PALETTE_RAM_MEM_SIZE))return (uint32_t)(Mem->DisMem->Palette_RAM) + (uint32_t)(((addr - PALETTE_RAM_ADDR_BASE) % PALETTE_RAM_MEM_SIZE));
            break;
        case VRAM_ADDR_BASE:
            if((addr ^ VRAM_ADDR_BASE) < VRAM_MEM_SIZE)return (uint32_t)(Mem->DisMem->VRAM) + (uint32_t)(addr - VRAM_ADDR_BASE);
            if(addr >= (VRAM_ADDR_BASE + VRAM_MEM_SIZE) && addr <= (VRAM_ADDR_BASE + 0x1FFFF))return (uint32_t)(Mem->DisMem->VRAM) + (uint32_t)((addr - 0x18000));
            if(addr >= (VRAM_ADDR_BASE + VRAM_MEM_SIZE))return (uint32_t)(Mem->DisMem->VRAM) + (uint32_t)(((addr - VRAM_ADDR_BASE) % 0x20000));
            break;
        case OAM_ADDR_BASE:
            if((addr ^ OAM_ADDR_BASE) < OAM_MEM_SIZE)return (uint32_t)(Mem->DisMem->OAM) + (uint32_t)(addr - OAM_ADDR_BASE);
            if(addr >= (OAM_ADDR_BASE + OAM_MEM_SIZE))return (uint32_t)(Mem->DisMem->OAM) + (uint32_t)(((addr - OAM_ADDR_BASE) % OAM_MEM_SIZE));
            break;
        case GAMEROM1_ADDR_BASE:
            if((addr ^ GAMEROM1_ADDR_BASE) < 0x9000000)return (uint32_t)((Mem->ExMem->Game_ROM1) + (addr - GAMEROM1_ADDR_BASE));
            break;
        case 0x9000000:
            if((addr ^ 0x9000000) < 0xA000000)return (uint32_t)((Mem->ExMem->Game_ROM1) + (addr - GAMEROM1_ADDR_BASE));
            break;
        case 0xA000000:
            if((addr ^ 0xA000000) < 0xB000000)return (uint32_t)((Mem->ExMem->Game_ROM2) + (addr - GAMEROM2_ADDR_BASE));
            break;
        case 0xB000000:
            if((addr ^ 0xB000000) < 0xC000000)return (uint32_t)((Mem->ExMem->Game_ROM2) + (addr - GAMEROM2_ADDR_BASE));
            break;
        case 0xC000000:
            if((addr ^ 0xC000000) < 0xD000000)return (uint32_t)((Mem->ExMem->Game_ROM3) + (addr - GAMEROM3_ADDR_BASE));
            break;
        case 0xD000000:
            if((addr ^ 0xD000000) < 0xE000000)return (uint32_t)((Mem->ExMem->Game_ROM3) + (addr - GAMEROM3_ADDR_BASE));
            break;
        case GAME_SRAM_MEM_SIZE:
            if((addr ^ GAMEROM_SDRAM_BASE) < GAME_SRAM_MEM_SIZE)return (uint32_t)(Mem->ExMem->Game_SRAM) + (uint32_t)(addr - GAMEROM_SDRAM_BASE);
            break;
        default:
            printf("Memory Address %08x Not Used", addr);
            return 1;
    }
}

