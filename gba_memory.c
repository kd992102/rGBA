/**
 * GBA 記憶體管理實現
 * 負責所有記憶體訪問、等待狀態計算
 */

#include "gba_core.h"
#include <string.h>
#include <stdio.h>

// 先行宣告 I/O 暫存器寫入輔助函式
static void GBA_IORegisterWrite32(GBA_Core *core, uint32_t address, uint32_t value);
static void GBA_IORegisterWrite16(GBA_Core *core, uint32_t address, uint16_t value);
static void GBA_IORegisterWrite8(GBA_Core *core, uint32_t address, uint8_t value);

/* ============================================================================
 * 記憶體訪問週期計算
 * ============================================================================ */

uint8_t GBA_MemoryGetAccessCycles(GBA_Core *core, uint32_t address, bool sequential) {
    MemoryRegion region = GBA_MemoryGetRegion(address);
    
    switch (region) {
        case MEM_REGION_BIOS:
            return 1;  // BIOS: 1 cycle (GBATEK)
            
        case MEM_REGION_EWRAM:
            // EWRAM: 16-bit bus, 3/3/6 for 8/16/32-bit (GBATEK)
            return 3;  // 簡化版：固定 3 cycles
            
        case MEM_REGION_IWRAM:
            return 1;  // IWRAM: 1 cycle (GBATEK)
            
        case MEM_REGION_IO:
            return 1;  // I/O: 1 cycle (GBATEK)
            
        case MEM_REGION_PALETTE:
        case MEM_REGION_VRAM:
        case MEM_REGION_OAM:
            // Video RAM: 16-bit bus, 1/1/2 for 8/16/32-bit (GBATEK)
            return 1;  // 簡化版：固定 1 cycle (不考慮 32-bit 和 PPU 衝突)
            
        case MEM_REGION_ROM_WS0:
            return sequential ? core->memory.wait_states.ws0_s : 
                                core->memory.wait_states.ws0_n;
            
        case MEM_REGION_ROM_WS1:
            return sequential ? core->memory.wait_states.ws1_s : 
                                core->memory.wait_states.ws1_n;
            
        case MEM_REGION_ROM_WS2:
            return sequential ? core->memory.wait_states.ws2_s : 
                                core->memory.wait_states.ws2_n;
            
        case MEM_REGION_SRAM:
            return core->memory.wait_states.sram;
            
        default:
            return 1;
    }
}

uint8_t* GBA_MemoryGetRegionPointer(GBA_Core *core, MemoryRegion region) {
    switch (region) {
        case MEM_REGION_BIOS:    return core->memory.bios;
        case MEM_REGION_EWRAM:   return core->memory.ewram;
        case MEM_REGION_IWRAM:   return core->memory.iwram;
        case MEM_REGION_IO:      return core->memory.io_registers;
        case MEM_REGION_PALETTE: return core->memory.palette;
        case MEM_REGION_VRAM:    return core->memory.vram;
        case MEM_REGION_OAM:     return core->memory.oam;
        case MEM_REGION_ROM_WS0:
        case MEM_REGION_ROM_WS1:
        case MEM_REGION_ROM_WS2: return core->memory.rom;
        case MEM_REGION_SRAM:    return core->memory.sram;
        default:                 return NULL;
    }
}

/* ============================================================================
 * 32 位讀取
 * ============================================================================ */

uint32_t GBA_MemoryRead32(GBA_Core *core, uint32_t address) {
    address &= ~3;  // 對齊到 4 位元組
    
    MemoryRegion region = GBA_MemoryGetRegion(address);
    uint8_t *ptr = NULL;
    uint32_t offset = 0;
    
    switch (region) {
        case MEM_REGION_BIOS:
            // BIOS 保護：只能在 PC < 0x4000 時讀取
            if (core->cpu.regs.pc < 0x4000) {
                ptr = core->memory.bios;
                offset = address & 0x3FFF;
            } else {
                // 非法訪問，返回最後讀取的值（簡化為 0）
                return 0xE129F000;  // 未定義指令
            }
            break;
            
        case MEM_REGION_EWRAM:
            ptr = core->memory.ewram;
            offset = address & 0x3FFFF;
            break;
            
        case MEM_REGION_IWRAM:
            ptr = core->memory.iwram;
            offset = address & 0x7FFF;
            break;
            
        case MEM_REGION_IO:
            ptr = core->memory.io_registers;
            offset = address & 0x3FF;
            // TODO: 特殊處理一些只讀暫存器
            break;
            
        case MEM_REGION_PALETTE:
            ptr = core->memory.palette;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_VRAM:
            ptr = core->memory.vram;
            offset = address & 0x1FFFF;
            // VRAM 映象處理
            if (offset >= 0x18000) {
                offset -= 0x8000;
            }
            break;
            
        case MEM_REGION_OAM:
            ptr = core->memory.oam;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_ROM_WS0:
        case MEM_REGION_ROM_WS1:
        case MEM_REGION_ROM_WS2:
            if (core->memory.rom && core->memory.rom_size > 0) {
                ptr = core->memory.rom;
                // 所有 ROM 視窗對映到同一 ROM
                offset = (address & 0x01FFFFFF) % core->memory.rom_size;
            } else {
                return 0xFFFFFFFF;  // 未載入 ROM
            }
            break;
            
        case MEM_REGION_SRAM:
            // SRAM 只支援 8 位訪問
            return core->memory.sram[address & 0xFFFF] * 0x01010101;
            
        default:
            // 未對映區域
            return 0;
    }
    
    // 讀取 32 位值（小端序）
    if (ptr) {
        return ptr[offset] | 
               (ptr[offset + 1] << 8) | 
               (ptr[offset + 2] << 16) | 
               (ptr[offset + 3] << 24);
    }
    
    return 0;
}

/* ============================================================================
 * 16 位讀取
 * ============================================================================ */

uint16_t GBA_MemoryRead16(GBA_Core *core, uint32_t address) {
    address &= ~1;  // 對齊到 2 位元組
    
    MemoryRegion region = GBA_MemoryGetRegion(address);
    uint8_t *ptr = NULL;
    uint32_t offset = 0;
    
    switch (region) {
        case MEM_REGION_BIOS:
            if (core->cpu.regs.pc < 0x4000) {
                ptr = core->memory.bios;
                offset = address & 0x3FFF;
            } else {
                return 0xF000;
            }
            break;
            
        case MEM_REGION_EWRAM:
            ptr = core->memory.ewram;
            offset = address & 0x3FFFF;
            break;
            
        case MEM_REGION_IWRAM:
            ptr = core->memory.iwram;
            offset = address & 0x7FFF;
            break;
            
        case MEM_REGION_IO:
            ptr = core->memory.io_registers;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_PALETTE:
            ptr = core->memory.palette;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_VRAM:
            ptr = core->memory.vram;
            offset = address & 0x1FFFF;
            if (offset >= 0x18000) {
                offset -= 0x8000;
            }
            break;
            
        case MEM_REGION_OAM:
            ptr = core->memory.oam;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_ROM_WS0:
        case MEM_REGION_ROM_WS1:
        case MEM_REGION_ROM_WS2:
            if (core->memory.rom && core->memory.rom_size > 0) {
                ptr = core->memory.rom;
                offset = (address & 0x01FFFFFF) % core->memory.rom_size;
            } else {
                return 0xFFFF;
            }
            break;
            
        case MEM_REGION_SRAM:
            return core->memory.sram[address & 0xFFFF] * 0x0101;
            
        default:
            return 0;
    }
    
    if (ptr) {
        return ptr[offset] | (ptr[offset + 1] << 8);
    }
    
    return 0;
}

/* ============================================================================
 * 8 位讀取
 * ============================================================================ */

uint8_t GBA_MemoryRead8(GBA_Core *core, uint32_t address) {
    MemoryRegion region = GBA_MemoryGetRegion(address);
    
    switch (region) {
        case MEM_REGION_BIOS:
            if (core->cpu.regs.pc < 0x4000) {
                return core->memory.bios[address & 0x3FFF];
            }
            return 0;
            
        case MEM_REGION_EWRAM:
            return core->memory.ewram[address & 0x3FFFF];
            
        case MEM_REGION_IWRAM:
            return core->memory.iwram[address & 0x7FFF];
            
        case MEM_REGION_IO:
            return core->memory.io_registers[address & 0x3FF];
            
        case MEM_REGION_PALETTE:
            return core->memory.palette[address & 0x3FF];
            
        case MEM_REGION_VRAM:
        {
            uint32_t offset = address & 0x1FFFF;
            if (offset >= 0x18000) offset -= 0x8000;
            return core->memory.vram[offset];
        }
            
        case MEM_REGION_OAM:
            return core->memory.oam[address & 0x3FF];
            
        case MEM_REGION_ROM_WS0:
        case MEM_REGION_ROM_WS1:
        case MEM_REGION_ROM_WS2:
            if (core->memory.rom && core->memory.rom_size > 0) {
                uint32_t offset = (address & 0x01FFFFFF) % core->memory.rom_size;
                return core->memory.rom[offset];
            }
            return 0xFF;
            
        case MEM_REGION_SRAM:
            return core->memory.sram[address & 0xFFFF];
            
        default:
            return 0;
    }
}

/* ============================================================================
 * 32 位寫入
 * ============================================================================ */

void GBA_MemoryWrite32(GBA_Core *core, uint32_t address, uint32_t value) {
    address &= ~3;
    
    MemoryRegion region = GBA_MemoryGetRegion(address);
    uint8_t *ptr = NULL;
    uint32_t offset = 0;
    
    switch (region) {
        case MEM_REGION_BIOS:
            return;  // BIOS 只讀
            
        case MEM_REGION_EWRAM:
            ptr = core->memory.ewram;
            offset = address & 0x3FFFF;
            break;
            
        case MEM_REGION_IWRAM:
            ptr = core->memory.iwram;
            offset = address & 0x7FFF;
            break;
            
        case MEM_REGION_IO:
            // I/O 暫存器需要特殊處理
            GBA_IORegisterWrite32(core, address, value);
            return;
            
        case MEM_REGION_PALETTE:
            ptr = core->memory.palette;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_VRAM:
            ptr = core->memory.vram;
            offset = address & 0x1FFFF;
            if (offset >= 0x18000) offset -= 0x8000;
            break;
            
        case MEM_REGION_OAM:
            ptr = core->memory.oam;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_ROM_WS0:
        case MEM_REGION_ROM_WS1:
        case MEM_REGION_ROM_WS2:
            return;  // ROM 只讀
            
        case MEM_REGION_SRAM:
            // SRAM 只支援 8 位寫入
            core->memory.sram[address & 0xFFFF] = value & 0xFF;
            return;
            
        default:
            return;
    }
    
    if (ptr) {
        ptr[offset]     = value & 0xFF;
        ptr[offset + 1] = (value >> 8) & 0xFF;
        ptr[offset + 2] = (value >> 16) & 0xFF;
        ptr[offset + 3] = (value >> 24) & 0xFF;
    }
}

/* ============================================================================
 * 16 位寫入
 * ============================================================================ */

void GBA_MemoryWrite16(GBA_Core *core, uint32_t address, uint16_t value) {
    address &= ~1;
    
    MemoryRegion region = GBA_MemoryGetRegion(address);
    uint8_t *ptr = NULL;
    uint32_t offset = 0;
    
    switch (region) {
        case MEM_REGION_BIOS:
            return;
            
        case MEM_REGION_EWRAM:
            ptr = core->memory.ewram;
            offset = address & 0x3FFFF;
            break;
            
        case MEM_REGION_IWRAM:
            ptr = core->memory.iwram;
            offset = address & 0x7FFF;
            break;
            
        case MEM_REGION_IO:
            GBA_IORegisterWrite16(core, address, value);
            return;
            
        case MEM_REGION_PALETTE:
            ptr = core->memory.palette;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_VRAM:
            ptr = core->memory.vram;
            offset = address & 0x1FFFF;
            if (offset >= 0x18000) offset -= 0x8000;
            break;
            
        case MEM_REGION_OAM:
            ptr = core->memory.oam;
            offset = address & 0x3FF;
            break;
            
        case MEM_REGION_SRAM:
            core->memory.sram[address & 0xFFFF] = value & 0xFF;
            return;
            
        default:
            return;
    }
    
    if (ptr) {
        ptr[offset]     = value & 0xFF;
        ptr[offset + 1] = (value >> 8) & 0xFF;
    }
}

/* ============================================================================
 * 8 位寫入
 * ============================================================================ */

void GBA_MemoryWrite8(GBA_Core *core, uint32_t address, uint8_t value) {
    MemoryRegion region = GBA_MemoryGetRegion(address);
    
    switch (region) {
        case MEM_REGION_BIOS:
            return;
            
        case MEM_REGION_EWRAM:
            core->memory.ewram[address & 0x3FFFF] = value;
            break;
            
        case MEM_REGION_IWRAM:
            core->memory.iwram[address & 0x7FFF] = value;
            break;
            
        case MEM_REGION_IO:
            GBA_IORegisterWrite8(core, address, value);
            break;
            
        case MEM_REGION_PALETTE:
        case MEM_REGION_VRAM:
        case MEM_REGION_OAM:
            // 這些區域不支援 8 位寫入
            break;
            
        case MEM_REGION_SRAM:
            core->memory.sram[address & 0xFFFF] = value;
            break;
            
        default:
            break;
    }
}

/* ============================================================================
 * I/O 暫存器處理（簡化版）
 * ============================================================================ */

static void GBA_IORegisterWrite32(GBA_Core *core, uint32_t address, uint32_t value) {
    GBA_IORegisterWrite16(core, address, value & 0xFFFF);
    GBA_IORegisterWrite16(core, address + 2, value >> 16);
}

static void GBA_IORegisterWrite16(GBA_Core *core, uint32_t address, uint16_t value) {
    uint32_t offset = address & 0x3FF;
    
    // 直接寫入（先）
    core->memory.io_registers[offset] = value & 0xFF;
    core->memory.io_registers[offset + 1] = value >> 8;
    
    // 特殊處理某些暫存器
    switch (offset) {
        case 0x200:  // IE
            core->interrupt.IE = value;
            break;
            
        case 0x202:  // IF (寫 1 清除)
            core->interrupt.IF &= ~value;
            break;
            
        case 0x204:  // WAITCNT
            // 更新等待狀態
            core->memory.wait_states.sram = (value & 0x3) + 1;
            core->memory.wait_states.ws0_n = ((value >> 2) & 0x3) + 1;
            core->memory.wait_states.ws0_s = ((value >> 4) & 0x1) ? 1 : 2;
            core->memory.wait_states.ws1_n = ((value >> 5) & 0x3) + 1;
            core->memory.wait_states.ws1_s = ((value >> 7) & 0x1) ? 1 : 4;
            core->memory.wait_states.ws2_n = ((value >> 8) & 0x3) + 1;
            core->memory.wait_states.ws2_s = ((value >> 10) & 0x1) ? 1 : 8;
            break;
            
        case 0x208:  // IME
            core->interrupt.IME = value & 1;
            break;
    }
}

static void GBA_IORegisterWrite8(GBA_Core *core, uint32_t address, uint8_t value) {
    uint32_t offset = address & 0x3FF;
    core->memory.io_registers[offset] = value;
    
    // 某些暫存器需要 16 位處理
    if (offset == 0x200 || offset == 0x201) {
        uint16_t ie = core->memory.io_registers[0x200] | 
                      (core->memory.io_registers[0x201] << 8);
        core->interrupt.IE = ie;
    }
}
