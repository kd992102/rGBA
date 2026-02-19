/**
 * GBA 核心系统实现
 * 负责整个模拟器的生命周期管理和主循环
 */

#include "gba_core.h"
#include "gba_cpu_instructions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 核心创建与销毁
 * ============================================================================ */

GBA_Core* GBA_CoreCreate(void) {
    GBA_Core *core = (GBA_Core*)calloc(1, sizeof(GBA_Core));
    if (!core) {
        fprintf(stderr, "Failed to allocate GBA_Core\n");
        return NULL;
    }
    
    // 分配内存区域
    core->memory.bios = (uint8_t*)malloc(16 * 1024);
    core->memory.ewram = (uint8_t*)malloc(256 * 1024);
    core->memory.iwram = (uint8_t*)malloc(32 * 1024);
    core->memory.io_registers = (uint8_t*)calloc(1024, 1);
    core->memory.palette = (uint8_t*)malloc(1024);
    core->memory.vram = (uint8_t*)malloc(96 * 1024);
    core->memory.oam = (uint8_t*)malloc(1024);
    core->memory.sram = (uint8_t*)malloc(64 * 1024);
    
    // 检查分配是否成功
    if (!core->memory.bios || !core->memory.ewram || !core->memory.iwram ||
        !core->memory.io_registers || !core->memory.palette || 
        !core->memory.vram || !core->memory.oam || !core->memory.sram) {
        fprintf(stderr, "Failed to allocate memory regions\n");
        GBA_CoreDestroy(core);
        return NULL;
    }
    
    // 分配帧缓冲 (240 × 160 × BGRA)
    core->ppu.framebuffer = (uint32_t*)malloc(240 * 160 * sizeof(uint32_t));
    if (!core->ppu.framebuffer) {
        fprintf(stderr, "Failed to allocate framebuffer\n");
        GBA_CoreDestroy(core);
        return NULL;
    }
    
    // 初始化为黑色
    memset(core->ppu.framebuffer, 0, 240 * 160 * sizeof(uint32_t));
    
    // 设置默认等待状态（参考 GBATEK）
    core->memory.wait_states.sram = 4;
    core->memory.wait_states.ws0_n = 4;
    core->memory.wait_states.ws0_s = 2;
    core->memory.wait_states.ws1_n = 4;
    core->memory.wait_states.ws1_s = 4;
    core->memory.wait_states.ws2_n = 4;
    core->memory.wait_states.ws2_s = 4;
    
    // 初始化 DMA
    core->dma.active_channel = -1;
    
    // 重置到初始状态
    GBA_CoreReset(core);
    
    printf("GBA Core created successfully\n");
    
    return core;
}

void GBA_CoreDestroy(GBA_Core *core) {
    if (!core) return;
    
    free(core->memory.bios);
    free(core->memory.ewram);
    free(core->memory.iwram);
    free(core->memory.io_registers);
    free(core->memory.palette);
    free(core->memory.vram);
    free(core->memory.oam);
    free(core->memory.rom);
    free(core->memory.sram);
    free(core->ppu.framebuffer);
    free(core);
    
    printf("GBA Core destroyed\n");
}

void GBA_CoreReset(GBA_Core *core) {
    if (!core) return;
    
    printf("Resetting GBA Core...\n");
    
    // 重置 CPU
    memset(&core->cpu, 0, sizeof(GBA_CPU));
    core->cpu.regs.cpsr = CPU_STATE_SYSTEM | (1 << 7);  // 系统模式，IRQ 禁用
    core->cpu.regs.exec_addr = 0x00000000;              // BIOS 入口（执行地址）
    core->cpu.regs.pc = 0x00000008;                     // PC = exec_addr + 8 (ARM)
    core->cpu.regs.exec_mode = CPU_MODE_ARM;
    core->cpu.regs.priv_mode = CPU_STATE_SYSTEM;
    
    // 设置堆栈指针（参考 GBATEK）
    core->cpu.regs.r[13] = 0x03007F00;        // SP_sys
    core->cpu.regs.irq.r13 = 0x03007FA0;      // SP_irq
    core->cpu.regs.svc.r13 = 0x03007FE0;      // SP_svc

    // 同步 User/System 寄存器快照
    core->cpu.regs.usr.r8_usr = core->cpu.regs.r[8];
    core->cpu.regs.usr.r9_usr = core->cpu.regs.r[9];
    core->cpu.regs.usr.r10_usr = core->cpu.regs.r[10];
    core->cpu.regs.usr.r11_usr = core->cpu.regs.r[11];
    core->cpu.regs.usr.r12_usr = core->cpu.regs.r[12];
    core->cpu.regs.usr.r13_usr = core->cpu.regs.r[13];
    core->cpu.regs.usr.r14_usr = core->cpu.regs.r[14];
    
    // 清空内存
    if (core->memory.ewram) memset(core->memory.ewram, 0, 256 * 1024);
    if (core->memory.iwram) memset(core->memory.iwram, 0, 32 * 1024);
    if (core->memory.io_registers) memset(core->memory.io_registers, 0, 1024);
    if (core->memory.palette) memset(core->memory.palette, 0, 1024);
    if (core->memory.vram) memset(core->memory.vram, 0, 96 * 1024);
    if (core->memory.oam) memset(core->memory.oam, 0, 1024);
    if (core->memory.sram) memset(core->memory.sram, 0xFF, 64 * 1024);
    
    // 重置子系统
    memset(&core->interrupt, 0, sizeof(GBA_Interrupt));
    memset(&core->timer, 0, sizeof(GBA_Timer));
    memset(&core->dma, 0, sizeof(GBA_DMA));
    memset(&core->ppu, 0, sizeof(GBA_PPU));
    
    // 重新设置帧缓冲指针（因为 memset 会清空）
    core->ppu.framebuffer = (uint32_t*)malloc(240 * 160 * sizeof(uint32_t));
    memset(core->ppu.framebuffer, 0, 240 * 160 * sizeof(uint32_t));
    
    core->dma.active_channel = -1;
    
    // 填充流水线
    GBA_CPUFlushPipeline(core);
    
    // 重置全局状态
    core->state.running = true;
    core->state.paused = false;
    core->state.frame_number = 0;
    
    printf("GBA Core reset complete\n");
}

/* ============================================================================
 * ROM/BIOS 载入
 * ============================================================================ */

int GBA_CoreLoadBIOS(GBA_Core *core, const uint8_t *data, size_t size) {
    if (!core || !data) {
        fprintf(stderr, "Invalid parameters for BIOS load\n");
        return -1;
    }
    
    if (size != 16384) {
        fprintf(stderr, "Invalid BIOS size: %zu (expected 16384)\n", size);
        return -1;
    }
    
    memcpy(core->memory.bios, data, 16384);
    printf("BIOS loaded successfully (16 KB)\n");
    
    return 0;
}

int GBA_CoreLoadROM(GBA_Core *core, const uint8_t *data, size_t size) {
    if (!core || !data || size == 0) {
        fprintf(stderr, "Invalid parameters for ROM load\n");
        return -1;
    }
    
    // 限制 ROM 大小到 32 MB
    if (size > 32 * 1024 * 1024) {
        fprintf(stderr, "ROM too large, truncating to 32 MB\n");
        size = 32 * 1024 * 1024;
    }
    
    // 释放旧 ROM
    if (core->memory.rom) {
        free(core->memory.rom);
    }
    
    // 分配新 ROM
    core->memory.rom = (uint8_t*)malloc(size);
    if (!core->memory.rom) {
        fprintf(stderr, "Failed to allocate ROM memory\n");
        return -1;
    }
    
    memcpy(core->memory.rom, data, size);
    core->memory.rom_size = size;
    
    // 打印 ROM 信息
    char title[13];
    memcpy(title, core->memory.rom + 0xA0, 12);
    title[12] = '\0';
    
    printf("ROM loaded successfully\n");
    printf("  Size: %zu bytes (%.2f MB)\n", size, size / (1024.0 * 1024.0));
    printf("  Title: %s\n", title);
    printf("  Game Code: %.4s\n", core->memory.rom + 0xAC);
    printf("  Maker Code: %.2s\n", core->memory.rom + 0xB0);
    
    return 0;
}

/* ============================================================================
 * 主循环
 * ============================================================================ */

uint32_t GBA_CoreRunFrame(GBA_Core *core) {
    if (!core) return 0;
    
    const uint32_t CYCLES_PER_FRAME = 280896;
    
    core->cpu.cycles.this_frame = 0;
    
    while (core->cpu.cycles.this_frame < CYCLES_PER_FRAME) {
        // 执行一条指令
        InstructionResult result = GBA_CPUStep(core);
        
        // 更新子系统
        GBA_PPUUpdate(core, result.cycles);
        GBA_TimerUpdate(core, result.cycles);
        
        // DMA 有最高优先级
        if (GBA_DMAIsActive(core)) {
            GBA_DMAUpdate(core);
        }
        
        // 检查是否需要暂停
        if (core->state.paused || !core->state.running) {
            break;
        }
    }
    
    core->state.frame_number++;
    
    return core->cpu.cycles.this_frame;
}

uint32_t GBA_CoreRunCycles(GBA_Core *core, uint32_t cycles) {
    if (!core) return 0;
    
    uint32_t executed = 0;
    uint32_t target = core->cpu.cycles.this_frame + cycles;
    
    while (core->cpu.cycles.this_frame < target) {
        InstructionResult result = GBA_CPUStep(core);
        
        executed += result.cycles;
        
        GBA_PPUUpdate(core, result.cycles);
        GBA_TimerUpdate(core, result.cycles);
        
        if (GBA_DMAIsActive(core)) {
            GBA_DMAUpdate(core);
        }
        
        if (core->state.paused || !core->state.running) {
            break;
        }
    }
    
    return executed;
}

const uint32_t* GBA_CoreGetFramebuffer(const GBA_Core *core) {
    return core ? core->ppu.framebuffer : NULL;
}

/* ============================================================================
 * 调试接口
 * ============================================================================ */

void GBA_CoreDumpState(const GBA_Core *core) {
    if (!core) return;
    
    printf("\n========== GBA Core State Dump ==========\n");
    printf("Frame: %llu\n", (unsigned long long)core->state.frame_number);
    printf("Total Cycles: %llu\n", (unsigned long long)core->cpu.cycles.total);
    printf("Frame Cycles: %u / 280896\n", core->cpu.cycles.this_frame);
    
    printf("\n--- CPU State ---\n");
    printf("PC: 0x%08X\n", core->cpu.regs.pc);
    printf("CPSR: 0x%08X [", core->cpu.regs.cpsr);
    if (GBA_CPU_GetFlag_N(&core->cpu)) printf("N");
    if (GBA_CPU_GetFlag_Z(&core->cpu)) printf("Z");
    if (GBA_CPU_GetFlag_C(&core->cpu)) printf("C");
    if (GBA_CPU_GetFlag_V(&core->cpu)) printf("V");
    printf("]\n");
    printf("Mode: %s\n", core->cpu.regs.exec_mode == CPU_MODE_ARM ? "ARM" : "THUMB");
    
    printf("\n--- Registers ---\n");
    for (int i = 0; i < 13; i++) {
        printf("R%-2d: 0x%08X  ", i, core->cpu.regs.r[i]);
        if ((i + 1) % 4 == 0) printf("\n");
    }
    printf("SP:  0x%08X\n", core->cpu.regs.r[13]);
    printf("LR:  0x%08X\n", core->cpu.regs.r[14]);
    
    printf("\n--- PPU State ---\n");
    printf("Scanline: %u\n", core->ppu.current_scanline);
    printf("VCOUNT: %u\n", core->memory.io_registers[0x006]);
    printf("VBlank: %s\n", core->ppu.in_vblank ? "Yes" : "No");
    
    printf("\n--- Interrupts ---\n");
    printf("IME: 0x%04X\n", core->interrupt.IME);
    printf("IE:  0x%04X\n", core->interrupt.IE);
    printf("IF:  0x%04X\n", core->interrupt.IF);
    
    printf("=========================================\n\n");
}
