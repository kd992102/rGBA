#include <stdint.h>
#include <stdio.h>

/*uint32_t wait_table_seq[16] = { // Default values
    0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 4
};
uint32_t wait_table_nonseq[16] = {
    0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4
};

const uint16_t mem_bus_is_16[16] = { // 1 if the address range has a 16-bit bus
    0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1
};

//void MemoryAccessCyclesUpdate(void);

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
}*/

uint32_t MemoryAddrReloc(uint32_t addr);

uint32_t MemRead32(uint32_t addr);
uint16_t MemRead16(uint32_t addr);
uint8_t MemRead8(uint32_t addr);
void MemWrite32(uint32_t addr, uint32_t data);
void MemWrite16(uint32_t addr, uint16_t data);
void MemWrite8(uint32_t addr, uint8_t data);
void PPUMemWrite16(uint32_t addr, uint16_t data);