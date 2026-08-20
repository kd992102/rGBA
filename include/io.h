#include <stdint.h>
#include <stdio.h>
#include "gba.h"

extern uint32_t wait_table_seq[16];
extern uint32_t wait_table_nonseq[16];
extern const uint16_t mem_bus_is_16[16];

void MemoryAccessCyclesUpdate(void);

static inline uint32_t MemoryGetAccessCycles(uint32_t seq, uint32_t is_32bit, uint32_t address)
{
    uint32_t index = (address >> 24) & 0xF;
    uint32_t doubletime = is_32bit && mem_bus_is_16[index];
    if (seq)
    {
        return (wait_table_seq[index] + 1) << doubletime;
    }
    else
    {
        if (doubletime)
            return (wait_table_seq[index] + wait_table_nonseq[index] + 2);
        else
            return (wait_table_nonseq[index] + 1);
    }
}

static inline uint32_t MemoryGetAccessCyclesNoSeq(uint32_t is_32bit, uint32_t address)
{
    uint32_t index = (address >> 24) & 0xF;
    if (is_32bit && mem_bus_is_16[index])
        return (wait_table_seq[index] + wait_table_nonseq[index] + 2);
    else
        return (wait_table_nonseq[index] + 1);
}

static inline uint32_t MemoryGetAccessCyclesSeq(uint32_t is_32bit, uint32_t address)
{
    //if ((address & 0x1FFFF) == 0)
    //    return MemoryGetAccessCyclesNoSeq(is_32bit, address);
    uint32_t index = (address >> 24) & 0xF;
    return (wait_table_seq[index] + 1) << (is_32bit && mem_bus_is_16[index]);
}

static inline uint32_t MemoryGetAccessCyclesNoSeq32(uint32_t address)
{
    uint32_t index = (address >> 24) & 0xF;
    if (mem_bus_is_16[index])
        return (wait_table_seq[index] + wait_table_nonseq[index] + 2);
    else
        return (wait_table_nonseq[index] + 1);
}

static inline uint32_t MemoryGetAccessCyclesNoSeq16(uint32_t address)
{
    return (wait_table_nonseq[(address >> 24) & 0xF] + 1);
}

static inline uint32_t MemoryGetAccessCyclesSeq32(uint32_t address)
{
    //if ((address & 0x1FFFF) == 0)
    //    return MemoryGetAccessCyclesNoSeq32(address);
    uint32_t index = (address >> 24) & 0xF;
    return (wait_table_seq[index] + 1) << (mem_bus_is_16[index]);
}

static inline uint32_t MemoryGetAccessCyclesSeq16(uint32_t address)
{
    //if ((address & 0x1FFFF) == 0)
    //    return MemoryGetAccessCyclesNoSeq16(address);
    return (wait_table_seq[(address >> 24) & 0xF] + 1);
}

uint8_t IO_Read8(GBA_Context *ctx, uint32_t addr);
uint16_t IO_Read16(GBA_Context *ctx, uint32_t addr);
uint32_t IO_Read32(GBA_Context *ctx, uint32_t addr);

void IO_Write8(GBA_Context *ctx, uint32_t addr, uint8_t data);
void IO_Write16(GBA_Context *ctx, uint32_t addr, uint16_t data);
void IO_Write32(GBA_Context *ctx, uint32_t addr, uint32_t data);

void AdvanceCycles(GBA_Context *ctx, uint32_t addr, int access_size, int seq);