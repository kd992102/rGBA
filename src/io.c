#include <stdio.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"
#include "gba.h"

uint32_t wait_table_seq[16] = { // Default values
    0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 4
};
uint32_t wait_table_nonseq[16] = {
    0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4
};
const uint16_t mem_bus_is_16[16] = { // 1 if the address range has a 16-bit bus
    0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1
};

// ---------------------------------------------------------
// Timing Model
// ---------------------------------------------------------

void AdvanceCycles(GBA_Context *ctx, uint32_t addr, int access_size, int seq) {
    // TODO: Calculate cycles based on wait states, access size, and sequential state
}

// ---------------------------------------------------------
// IO Handlers (0x04XXXXXX)
// ---------------------------------------------------------

uint8_t IO_Read8(GBA_Context *ctx, uint32_t addr) {
    uint32_t offset = addr & 0x3FFFF;
    switch (offset) {
        // TODO: Implement IO reads
        default:
            return 0;
    }
}

uint16_t IO_Read16(GBA_Context *ctx, uint32_t addr) {
    uint32_t offset = addr & 0x3FFFF;
    switch (offset) {
        // TODO: Implement IO reads
        default:
            return 0;
    }
}

uint32_t IO_Read32(GBA_Context *ctx, uint32_t addr) {
    return IO_Read16(ctx, addr) | (IO_Read16(ctx, addr + 2) << 16);
}

void IO_Write8(GBA_Context *ctx, uint32_t addr, uint8_t data) {
    uint32_t offset = addr & 0x3FFFF;
    switch (offset) {
        // TODO: Implement IO writes
        default:
            break;
    }
}

void IO_Write16(GBA_Context *ctx, uint32_t addr, uint16_t data) {
    uint32_t offset = addr & 0x3FFFF;
    switch (offset) {
        // TODO: Implement IO writes
        default:
            break;
    }
}

void IO_Write32(GBA_Context *ctx, uint32_t addr, uint32_t data) {
    IO_Write16(ctx, addr, data & 0xFFFF);
    IO_Write16(ctx, addr + 2, data >> 16);
}

// ---------------------------------------------------------
// Memory Read Handlers
// ---------------------------------------------------------

uint8_t MemRead8(GBA_Context *ctx, uint32_t addr) {
    AdvanceCycles(ctx, addr, 8, 0); // Temporary N-seq for all
    uint8_t region = addr >> 24;
    
    switch (region) {
        case 0x00: // BIOS
            return 0;
        case 0x02: // EWRAM
            return 0;
        case 0x03: // IWRAM
            return 0;
        case 0x04: // IO Registers
            return IO_Read8(ctx, addr);
        case 0x05: // Palette RAM
            return 0;
        case 0x06: // VRAM
            return 0;
        case 0x07: // OAM
            return 0;
        case 0x08: // ROM WS0
        case 0x09:
        case 0x0A: // ROM WS1
        case 0x0B:
        case 0x0C: // ROM WS2
        case 0x0D:
            return 0;
        case 0x0E: // SRAM
            return 0;
        default:
            return 0; // Open bus
    }
}

uint16_t MemRead16(GBA_Context *ctx, uint32_t addr) {
    AdvanceCycles(ctx, addr, 16, 0);
    uint8_t region = addr >> 24;
    
    switch (region) {
        case 0x00: // BIOS
            return 0;
        case 0x02: // EWRAM
            return 0;
        case 0x03: // IWRAM
            return 0;
        case 0x04: // IO Registers
            return IO_Read16(ctx, addr);
        case 0x05: // Palette RAM
            return 0;
        case 0x06: // VRAM
            return 0;
        case 0x07: // OAM
            return 0;
        case 0x08: // ROM WS0
        case 0x09:
        case 0x0A: // ROM WS1
        case 0x0B:
        case 0x0C: // ROM WS2
        case 0x0D:
            return 0;
        case 0x0E: // SRAM
            return 0;
        default:
            return 0; // Open bus
    }
}

uint32_t MemRead32(GBA_Context *ctx, uint32_t addr) {
    AdvanceCycles(ctx, addr, 32, 0);
    uint8_t region = addr >> 24;
    
    switch (region) {
        case 0x00: // BIOS
            return 0;
        case 0x02: // EWRAM
            return 0;
        case 0x03: // IWRAM
            return 0;
        case 0x04: // IO Registers
            return IO_Read32(ctx, addr);
        case 0x05: // Palette RAM
            return 0;
        case 0x06: // VRAM
            return 0;
        case 0x07: // OAM
            return 0;
        case 0x08: // ROM WS0
        case 0x09:
        case 0x0A: // ROM WS1
        case 0x0B:
        case 0x0C: // ROM WS2
        case 0x0D:
            return 0;
        case 0x0E: // SRAM
            return 0;
        default:
            return 0; // Open bus
    }
}

// ---------------------------------------------------------
// Memory Write Handlers
// ---------------------------------------------------------

void MemWrite8(GBA_Context *ctx, uint32_t addr, uint8_t data) {
    AdvanceCycles(ctx, addr, 8, 0);
    uint8_t region = addr >> 24;
    
    switch (region) {
        case 0x02: // EWRAM
            break;
        case 0x03: // IWRAM
            break;
        case 0x04: // IO Registers
            IO_Write8(ctx, addr, data);
            break;
        case 0x05: // Palette RAM
            break;
        case 0x06: // VRAM
            break;
        case 0x07: // OAM
            break;
        case 0x0E: // SRAM
            break;
        default:
            // ROM is read-only, BIOS is read-only
            break;
    }
}

void MemWrite16(GBA_Context *ctx, uint32_t addr, uint16_t data) {
    AdvanceCycles(ctx, addr, 16, 0);
    uint8_t region = addr >> 24;
    
    switch (region) {
        case 0x02: // EWRAM
            break;
        case 0x03: // IWRAM
            break;
        case 0x04: // IO Registers
            IO_Write16(ctx, addr, data);
            break;
        case 0x05: // Palette RAM
            break;
        case 0x06: // VRAM
            break;
        case 0x07: // OAM
            break;
        case 0x0E: // SRAM
            break;
        default:
            break;
    }
}

void MemWrite32(GBA_Context *ctx, uint32_t addr, uint32_t data) {
    AdvanceCycles(ctx, addr, 32, 0);
    uint8_t region = addr >> 24;
    
    switch (region) {
        case 0x02: // EWRAM
            break;
        case 0x03: // IWRAM
            break;
        case 0x04: // IO Registers
            IO_Write32(ctx, addr, data);
            break;
        case 0x05: // Palette RAM
            break;
        case 0x06: // VRAM
            break;
        case 0x07: // OAM
            break;
        case 0x0E: // SRAM
            break;
        default:
            break;
    }
}

void PPUMemWrite16(GBA_Context *ctx, uint32_t addr, uint16_t data){
    // TODO: PPU specific bypass for VRAM/OAM/Palette
}