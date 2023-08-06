#include <stdint.h>
#define __BASE_ADDR__ 0x00000000

#define BIOS_MEM_SIZE 0x4000
#define WRAM_BOARD_MEM_SIZE 0x40000
#define WRAM_CHIP_MEM_SIZE 0x8000
#define RESERVED_MEM_SIZE 0x1000
#define IO_REG_MEM_SIZE 0x800

#define PALETTE_RAM_MEM_SIZE 0x400
#define VRAM_MEM_SIZE 0x18000
#define OAM_MEM_SIZE 0x400

#define GAME_ROM_MAXSIZE 0x2000000
#define GAME_SRAM_MEM_SIZE 0x10000

#define BIOS_ADDR_BASE 0x00000000
#define WRAMB_ADDR_BASE 0x2000000
#define WRAMC_ADDR_BASE 0x3000000
#define RESERVED_ADDR_BASE 0x3fffe00
#define IOREG_ADDR_BASE 0x4000000

#define PALETTE_RAM_ADDR_BASE 0x5000000
#define VRAM_ADDR_BASE 0x6000000
#define OAM_ADDR_BASE 0x7000000

#define GAMEROM1_ADDR_BASE 0x8000000
#define GAMEROM2_ADDR_BASE 0xA000000
#define GAMEROM3_ADDR_BASE 0xC000000
#define GAMEROM_SDRAM_BASE 0xE000000

typedef struct Gba_Mem {
    uint32_t *BIOS;
    uint32_t *WRAM_board;
    uint32_t *WRAM_chip;
    uint32_t *Reserved;
    uint32_t *IO_Reg;//modified

    uint32_t *Palette_RAM;
    uint32_t *VRAM;
    uint32_t *OAM;

    uint8_t *Game_ROM1;
    uint8_t *Game_ROM2;
    uint8_t *Game_ROM3;
    uint8_t *Game_SRAM;
} GbaMem;

void Init_GbaMem();
void Release_GbaMem();