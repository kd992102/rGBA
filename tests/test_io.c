#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../include/gba.h"
#include "../include/io.h"

void test_memory_boundaries() {
    GBA_Context ctx;
    GBA_Init(&ctx);
    
    // Test EWRAM first and last byte
    MemWrite8(&ctx, 0x02000000, 0xAA);
    assert(MemRead8(&ctx, 0x02000000) == 0xAA);
    
    MemWrite8(&ctx, 0x0203FFFF, 0xBB);
    assert(MemRead8(&ctx, 0x0203FFFF) == 0xBB);

    printf("Boundary tests passed!\n");
}

void test_mirroring() {
    GBA_Context ctx;
    GBA_Init(&ctx);
    
    // Test EWRAM mirror: write to 0x02000000, read from 0x02040000
    MemWrite32(&ctx, 0x02000000, 0xDEADBEEF);
    assert(MemRead32(&ctx, 0x02040000) == 0xDEADBEEF);
    
    // Test VRAM mirror: write to 0x06000000, read from 0x06020000
    MemWrite32(&ctx, 0x06000000, 0xCAFEBABE);
    assert(MemRead32(&ctx, 0x06020000) == 0xCAFEBABE);

    printf("Mirror tests passed!\n");
}

void test_open_bus() {
    GBA_Context ctx;
    GBA_Init(&ctx);
    
    // Test reading invalid address
    uint32_t val = MemRead32(&ctx, 0x01000000); 
    assert(val == 0); // dummy_open_bus is 0
    
    printf("Open Bus tests passed!\n");
}

void test_io_masks() {
    GBA_Context ctx;
    GBA_Init(&ctx);
    
    // Test DISPSTAT mask (0x4000004)
    MemWrite16(&ctx, 0x4000004, 0xFFFF);
    uint16_t val = MemRead16(&ctx, 0x4000004);
    // DISPSTAT bits 0-2 are read-only (mostly set by PPU), so writing 0xFFFF shouldn't overwrite bits 0-2 from our side
    // Our mask logic in MemWrite16: ((data & 0xfff8)|(tmp & 0x7))
    assert((val & 0xFFF8) == 0xFFF8);

    printf("IO Masks tests passed!\n");
}

void test_rom_access() {
    GBA_Context ctx;
    GBA_Init(&ctx);
    
    // Setup dummy ROM
    ctx.mem.rom = (uint8_t*)malloc(1024);
    ctx.mem.rom_size = 1024;
    ctx.mem.rom[0] = 0x12;
    ctx.mem.rom[1] = 0x34;
    ctx.mem.rom[2] = 0x56;
    ctx.mem.rom[3] = 0x78;
    
    // Read from ROM
    assert(MemRead32(&ctx, 0x08000000) == 0x78563412); // Little endian
    
    free(ctx.mem.rom);
    printf("ROM access tests passed!\n");
}

int main() {
    test_memory_boundaries();
    test_mirroring();
    test_open_bus();
    test_io_masks();
    test_rom_access();
    
    printf("\nALL IO AND MEMORY TESTS PASSED!\n");
    return 0;
}
