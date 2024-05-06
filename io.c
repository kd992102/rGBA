#include <stdio.h>
#include "memory.h"
#include "io.h"
#include "arm7tdmi.h"

extern Gba_Cpu *cpu;
extern GbaMem *Mem;

uint32_t MemoryAddrReloc(uint32_t addr){
    if(addr >= 0x0 && addr <= 0xE010000){
    switch(addr & 0xf000000){
        case BIOS_ADDR_BASE:
            if((addr ^ BIOS_ADDR_BASE) < BIOS_MEM_SIZE)return (uint32_t)(Mem->BIOS) + (uint32_t)(addr - BIOS_ADDR_BASE);
            break;
        case WRAMB_ADDR_BASE:
            if((addr ^ WRAMB_ADDR_BASE) < WRAM_BOARD_MEM_SIZE)return (uint32_t)(Mem->WRAM_board) + (uint32_t)(addr -WRAMB_ADDR_BASE);
            if(addr >= (WRAMB_ADDR_BASE + WRAM_BOARD_MEM_SIZE))return (uint32_t)(Mem->WRAM_board) + (uint32_t)(((addr - WRAMC_ADDR_BASE) % WRAM_BOARD_MEM_SIZE));
            break;
        case WRAMC_ADDR_BASE:
            if((addr ^ WRAMC_ADDR_BASE) < WRAM_CHIP_MEM_SIZE)return (uint32_t)(Mem->WRAM_chip) + (uint32_t)(addr - WRAMC_ADDR_BASE);
            if(addr >= (WRAMC_ADDR_BASE + WRAM_CHIP_MEM_SIZE))return (uint32_t)(Mem->WRAM_chip) + (uint32_t)((addr - WRAMC_ADDR_BASE) % WRAM_CHIP_MEM_SIZE);
            break;
        case IOREG_ADDR_BASE:
            if((addr ^ IOREG_ADDR_BASE) < IO_REG_MEM_SIZE)return (uint32_t)(Mem->IO_Reg) + (uint32_t)(addr - IOREG_ADDR_BASE);
            if(addr >= (IOREG_ADDR_BASE + 0x800))return (uint32_t)(Mem->IO_Reg) + (uint32_t)(((addr - IOREG_ADDR_BASE) % 0x10000));
            break;
        case PALETTE_RAM_ADDR_BASE:
            if((addr ^ PALETTE_RAM_ADDR_BASE) < PALETTE_RAM_MEM_SIZE)return (uint32_t)(Mem->Palette_RAM) + (uint32_t)(addr - PALETTE_RAM_ADDR_BASE);
            if(addr >= (PALETTE_RAM_ADDR_BASE + PALETTE_RAM_MEM_SIZE))return (uint32_t)(Mem->Palette_RAM) + (uint32_t)(((addr - PALETTE_RAM_ADDR_BASE) % PALETTE_RAM_MEM_SIZE));
            break;
        case VRAM_ADDR_BASE:
            if((addr ^ VRAM_ADDR_BASE) < VRAM_MEM_SIZE)return (uint32_t)(Mem->VRAM) + (uint32_t)(addr - VRAM_ADDR_BASE);
            if(addr >= (VRAM_ADDR_BASE + VRAM_MEM_SIZE) && addr <= (VRAM_ADDR_BASE + 0x1FFFF))return (uint32_t)(Mem->VRAM) + (uint32_t)((addr - 0x18000));
            if(addr >= (VRAM_ADDR_BASE + VRAM_MEM_SIZE))return (uint32_t)(Mem->VRAM) + (uint32_t)(((addr - VRAM_ADDR_BASE) % 0x20000));
            break;
        case OAM_ADDR_BASE:
            if((addr ^ OAM_ADDR_BASE) < OAM_MEM_SIZE)return (uint32_t)(Mem->OAM) + (uint32_t)(addr - OAM_ADDR_BASE);
            if(addr >= (OAM_ADDR_BASE + OAM_MEM_SIZE))return (uint32_t)(Mem->OAM) + (uint32_t)(((addr - OAM_ADDR_BASE) % OAM_MEM_SIZE));
            break;
        case GAMEROM1_ADDR_BASE:
            //printf("GAME ROM %x\n", addr);
            if((addr ^ GAMEROM1_ADDR_BASE) < 0x1000000)return (uint32_t)((Mem->Game_ROM1) + (addr - GAMEROM1_ADDR_BASE));
            break;
        case 0x9000000:
            if((addr ^ 0x9000000) < 0x1000000)return (uint32_t)((Mem->Game_ROM1) + (addr - GAMEROM1_ADDR_BASE));
            break;
        case 0xA000000:
            if((addr ^ 0xA000000) < 0x1000000)return (uint32_t)((Mem->Game_ROM2) + (addr - GAMEROM2_ADDR_BASE));
            break;
        case 0xB000000:
            if((addr ^ 0xB000000) < 0x1000000)return (uint32_t)((Mem->Game_ROM2) + (addr - GAMEROM2_ADDR_BASE));
            break;
        case 0xC000000:
            if((addr ^ 0xC000000) < 0x1000000)return (uint32_t)((Mem->Game_ROM3) + (addr - GAMEROM3_ADDR_BASE));
            break;
        case 0xD000000:
            if((addr ^ 0xD000000) < 0x1000000)return (uint32_t)((Mem->Game_ROM3) + (addr - GAMEROM3_ADDR_BASE));
            break;
        case 0xE000000:
            //ErrorHandler(cpu);
            if((addr ^ GAMEROM_SDRAM_BASE) < GAME_SRAM_MEM_SIZE){
                return (uint32_t)(Mem->Game_SRAM) + (addr - GAMEROM_SDRAM_BASE);
            }
            break;
        default:
            printf("cycle %lld Memory Address %08x Not Used\n", cpu->cycle_sum, addr);
            ErrorHandler(cpu);
            return 1;
    }
    }
    else{
        printf("cycle %lld Memory Address %08x Not Used\n", cpu->cycle_sum, addr);
        ErrorHandler(cpu);
        return 1;
    }
}

uint32_t MemRead32(uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    if(addr >= 0x2000000 && addr <= 0x203FFFF)cpu->cycle += 5;
    if(addr >= 0x5000000 && addr <= 0x6017FFF)cpu->cycle += 1;
    if(addr >= 0x8000000 && addr <= 0xDFFFFFF){
        cpu->cycle += 4;
    }
    return *((uint32_t *)RelocAddr);
}

uint16_t MemRead16(uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    if(addr >= 0x2000000 && addr <= 0x203FFFF)cpu->cycle += 2;
    if(addr >= 0x8000000 && addr <= 0xDFFFFFF)cpu->cycle += 4;
    return *((uint16_t *)RelocAddr);
}

uint8_t MemRead8(uint32_t addr){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    if(addr >= 0x2000000 && addr <= 0x203FFFF)cpu->cycle += 2;
    if(addr >= 0x8000000 && addr <= 0xDFFFFFF)cpu->cycle += 4;
    return *((uint8_t *)RelocAddr);
}

void MemWrite32(uint32_t addr, uint32_t data){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    uint32_t tmp;

    if(addr >= 0x2000000 && addr <= 0x203FFFF)cpu->cycle += 5;
    if(addr >= 0x5000000 && addr <= 0x6017FFF)cpu->cycle += 1;
    if(addr >= 0x8000000 && addr <= 0xDFFFFFF)cpu->cycle += 4;
    if(addr == 0x4000004){
        tmp = *((uint32_t *)RelocAddr);
        *((uint32_t *)RelocAddr) = ((data & 0xff00fff8)|(tmp & 0xff0007));
    }
    else if(addr >= 0x4000200 && addr < 0x4000204){
        MemWrite8(addr,data & 0xff);
        MemWrite8(addr+1,(data >> 8) & 0xff);
        MemWrite8(addr+2,(data >> 16) & 0xff);
        MemWrite8(addr+3,(data >> 24) & 0xff);
    }
    else{*((uint32_t *)RelocAddr) = data;}
}

void MemWrite16(uint32_t addr, uint16_t data){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    uint16_t tmp;
    if(addr >= 0x2000000 && addr <= 0x203FFFF)cpu->cycle += 2;
    if(addr >= 0x8000000 && addr <= 0xDFFFFFF)cpu->cycle += 4;
    if(addr == 0x4000004){
        tmp = *((uint16_t *)RelocAddr);
        *((uint16_t *)RelocAddr) = ((data & 0xfff8)|(tmp & 0x7));
    }
    else if(addr == 0x4000006){
        tmp = *((uint16_t *)RelocAddr);
        *((uint16_t *)RelocAddr) = tmp;
    }
    else if(addr >= 0x4000202 && addr < 0x4000204){
        MemWrite8(addr,data & 0xff);
        MemWrite8(addr+1,(data >> 8) & 0xff);
    }
    else{
        *((uint16_t *)RelocAddr) = data;
    }
}

void MemWrite8(uint32_t addr, uint8_t data){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    uint8_t tmp;
    if(addr >= 0x2000000 && addr <= 0x203FFFF)cpu->cycle += 2;
    if(addr >= 0x8000000 && addr <= 0xDFFFFFF)cpu->cycle += 4;
    if(addr == 0x4000004){
        tmp = *((uint8_t *)RelocAddr);
        *((uint8_t *)RelocAddr) = ((data & 0xf8)|(tmp & 0x7));
    }
    else if(addr == 0x4000006){
        tmp = *((uint8_t *)RelocAddr);
        *((uint8_t *)RelocAddr) = tmp;
    }
    else if(addr >= 0x4000200 && addr < 0x4000208){
        if(addr == 0x4000202){
            tmp = *((uint8_t *)RelocAddr);
            *((uint8_t *)RelocAddr) = ((tmp & ~0xff)|((tmp & 0xff) & (~data)));
        }
        else if(addr == 0x4000203){
            tmp = *((uint8_t *)RelocAddr);
            *((uint8_t *)RelocAddr) = ((tmp & ~0xff)|((tmp & 0xff) & (~data)));
        }
        else{
            *((uint8_t *)RelocAddr) = data;
        }
    }
    else if(addr == 0x4000301){
        cpu->Halt = 1;
        //printf("CPU HALT\n");
    }
    else{*((uint8_t *)RelocAddr) = data;}
}

void PPUMemWrite16(uint32_t addr, uint16_t data){
    uint32_t RelocAddr = MemoryAddrReloc(addr);
    *((uint16_t *)RelocAddr) = data;
}