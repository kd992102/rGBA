#include <stdint.h>
#include <stdio.h>

uint32_t MemoryAddrReloc(uint32_t addr);

uint32_t MemRead32(uint32_t addr);
uint16_t MemRead16(uint32_t addr);
uint8_t MemRead8(uint32_t addr);
void MemWrite32(uint32_t addr, uint32_t data);
void MemWrite16(uint32_t addr, uint16_t data);
void MemWrite8(uint32_t addr, uint8_t data);
void PPUMemWrite16(uint32_t addr, uint16_t data);