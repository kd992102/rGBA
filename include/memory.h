#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#define __BASE__ 0x00000000

#define BIOS_SIZE 0x4000
#define WRAM_BOARD_SIZE 0x40000
#define WRAM_CHIP_SIZE 0x8000
#define RESERVED_SIZE 0x1000
#define IO_REG_SIZE 0x800

#define PALETTE_RAM_SIZE 0x400
#define VRAM_SIZE 0x18000
#define OAM_SIZE 0x400

#define GAME_ROM_MAXSIZE 0x2000000
#define GAME_SRAM_SIZE 0x10000

#define BIOS_BASE 0x00000000
#define WRAMB_BASE 0x2000000
#define WRAMC_BASE 0x3000000
#define RESERVED_BASE 0x3fffe00
#define IOREG_BASE 0x4000000

#define PALETTE_RAM_BASE 0x5000000
#define VRAM_BASE 0x6000000
#define OAM_BASE 0x7000000

#define ROM_WS1_BASE 0x8000000
#define ROM_WS2_BASE 0xA000000
#define ROM_WS3_BASE 0xC000000
#define SDRAM_BASE 0xE000000

typedef struct GbaMemory {
    uint8_t bios[BIOS_SIZE];
    uint8_t ewram[WRAM_BOARD_SIZE];
    uint8_t iwram[WRAM_CHIP_SIZE];
    uint8_t reserved[RESERVED_SIZE];
    uint8_t io[IO_REG_SIZE];

    uint8_t palatte[PALETTE_RAM_SIZE];
    uint8_t vram[VRAM_SIZE];
    uint8_t oam[OAM_SIZE];
    uint8_t sram[GAME_SRAM_SIZE];

    uint8_t *rom;
    uint32_t rom_size;
    uint32_t rom_mask;
} GbaMem;

void Memory_Init(GbaMem *mem);
void Memory_Reset(GbaMem *mem);
void Memory_Release(GbaMem *mem);

#endif