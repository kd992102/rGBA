/**
 * GBA CPU 执行引擎
 * 负责指令获取、流水线管理、中断处理
 */

#include "gba_core.h"
#include "gba_cpu_instructions.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * CPU 执行
 * ============================================================================ */

InstructionResult GBA_CPUStep(GBA_Core *core) {
    GBA_CPU *cpu = &core->cpu;
    InstructionResult result = {0};
    
    // 检查中断
    if (GBA_InterruptPending(core) && !cpu->state.halted) {
        GBA_CPUCheckInterrupts(core);
    }
    
    // 如果 CPU 暂停（HALT 状态），只消耗 1 周期
    if (cpu->state.halted) {
        result.cycles = 1;
        return result;
    }
    
    // 获取指令（从流水线）
    if (cpu->regs.exec_mode == CPU_MODE_ARM) {
        // ARM 模式（32 位指令）
        uint32_t instruction = cpu->regs.pipeline[0];
        
        // 推进流水线
        cpu->regs.pipeline[0] = cpu->regs.pipeline[1];
        cpu->regs.pipeline[1] = cpu->regs.pipeline[2];
        cpu->regs.pipeline[2] = GBA_MemoryRead32(core, cpu->regs.exec_addr + 12);
        
        // 执行指令
        result = GBA_CPU_ExecuteARM(core, instruction);
        
        // 更新 PC（如果没有分支）
        if (!result.branch_taken) {
            cpu->regs.exec_addr += 4;
            cpu->regs.pc += 4;
        }
        
    } else {
        // THUMB 模式（16 位指令）
        uint16_t instruction = cpu->regs.pipeline[0] & 0xFFFF;
        
        // 推进流水线
        cpu->regs.pipeline[0] = cpu->regs.pipeline[1];
        cpu->regs.pipeline[1] = cpu->regs.pipeline[2];
        cpu->regs.pipeline[2] = GBA_MemoryRead16(core, cpu->regs.exec_addr + 6);
        
        // 执行指令
        result = GBA_CPU_ExecuteTHUMB(core, instruction);
        
        // 更新 PC
        if (!result.branch_taken) {
            cpu->regs.exec_addr += 2;
            cpu->regs.pc += 2;
        }
    }
    
    // 累计周期
    cpu->cycles.total += result.cycles;
    cpu->cycles.this_frame += result.cycles;
    cpu->cycles.instruction = result.cycles;
    
    return result;
}

/* ============================================================================
 * 流水线管理
 * ============================================================================ */

void GBA_CPUFlushPipeline(GBA_Core *core) {
    GBA_CPU *cpu = &core->cpu;
    
    if (cpu->regs.exec_mode == CPU_MODE_ARM) {
        // ARM 模式
        cpu->regs.pipeline[0] = GBA_MemoryRead32(core, cpu->regs.exec_addr);
        cpu->regs.pipeline[1] = GBA_MemoryRead32(core, cpu->regs.exec_addr + 4);
        cpu->regs.pipeline[2] = GBA_MemoryRead32(core, cpu->regs.exec_addr + 8);
    } else {
        // THUMB 模式
        cpu->regs.pipeline[0] = GBA_MemoryRead16(core, cpu->regs.exec_addr);
        cpu->regs.pipeline[1] = GBA_MemoryRead16(core, cpu->regs.exec_addr + 2);
        cpu->regs.pipeline[2] = GBA_MemoryRead16(core, cpu->regs.exec_addr + 4);
    }
}

/* ============================================================================
 * 模式切换
 * ============================================================================ */

void GBA_CPU_SwitchMode(GBA_Core *core, CPU_PrivilegeMode new_mode) {
    GBA_CPU *cpu = &core->cpu;
    CPU_PrivilegeMode old_mode = cpu->regs.priv_mode;
    
    if (old_mode == new_mode) return;
    
    // 保存当前模式的银行寄存器
    if (old_mode != CPU_STATE_FIQ) {
        cpu->regs.usr.r8_usr = cpu->regs.r[8];
        cpu->regs.usr.r9_usr = cpu->regs.r[9];
        cpu->regs.usr.r10_usr = cpu->regs.r[10];
        cpu->regs.usr.r11_usr = cpu->regs.r[11];
        cpu->regs.usr.r12_usr = cpu->regs.r[12];
    }

    if (old_mode == CPU_STATE_USER || old_mode == CPU_STATE_SYSTEM) {
        cpu->regs.usr.r13_usr = cpu->regs.r[13];
        cpu->regs.usr.r14_usr = cpu->regs.r[14];
    }

    switch (old_mode) {
        case CPU_STATE_FIQ:
            cpu->regs.fiq.r8_fiq = cpu->regs.r[8];
            cpu->regs.fiq.r9_fiq = cpu->regs.r[9];
            cpu->regs.fiq.r10_fiq = cpu->regs.r[10];
            cpu->regs.fiq.r11_fiq = cpu->regs.r[11];
            cpu->regs.fiq.r12_fiq = cpu->regs.r[12];
            cpu->regs.fiq.r13_fiq = cpu->regs.r[13];
            cpu->regs.fiq.r14_fiq = cpu->regs.r[14];
            break;
            
        case CPU_STATE_IRQ:
            cpu->regs.irq.r13 = cpu->regs.r[13];
            cpu->regs.irq.r14 = cpu->regs.r[14];
            break;
            
        case CPU_STATE_SUPERVISOR:
            cpu->regs.svc.r13 = cpu->regs.r[13];
            cpu->regs.svc.r14 = cpu->regs.r[14];
            break;
            
        case CPU_STATE_ABORT:
            cpu->regs.abt.r13 = cpu->regs.r[13];
            cpu->regs.abt.r14 = cpu->regs.r[14];
            break;
            
        case CPU_STATE_UNDEFINED:
            cpu->regs.und.r13 = cpu->regs.r[13];
            cpu->regs.und.r14 = cpu->regs.r[14];
            break;
            
        default:
            break;
    }
    
    // 恢复新模式的银行寄存器
    if (new_mode != CPU_STATE_FIQ) {
        cpu->regs.r[8] = cpu->regs.usr.r8_usr;
        cpu->regs.r[9] = cpu->regs.usr.r9_usr;
        cpu->regs.r[10] = cpu->regs.usr.r10_usr;
        cpu->regs.r[11] = cpu->regs.usr.r11_usr;
        cpu->regs.r[12] = cpu->regs.usr.r12_usr;
    }

    switch (new_mode) {
        case CPU_STATE_FIQ:
            cpu->regs.r[8] = cpu->regs.fiq.r8_fiq;
            cpu->regs.r[9] = cpu->regs.fiq.r9_fiq;
            cpu->regs.r[10] = cpu->regs.fiq.r10_fiq;
            cpu->regs.r[11] = cpu->regs.fiq.r11_fiq;
            cpu->regs.r[12] = cpu->regs.fiq.r12_fiq;
            cpu->regs.r[13] = cpu->regs.fiq.r13_fiq;
            cpu->regs.r[14] = cpu->regs.fiq.r14_fiq;
            break;
            
        case CPU_STATE_IRQ:
            cpu->regs.r[13] = cpu->regs.irq.r13;
            cpu->regs.r[14] = cpu->regs.irq.r14;
            break;
            
        case CPU_STATE_SUPERVISOR:
            cpu->regs.r[13] = cpu->regs.svc.r13;
            cpu->regs.r[14] = cpu->regs.svc.r14;
            break;
            
        case CPU_STATE_ABORT:
            cpu->regs.r[13] = cpu->regs.abt.r13;
            cpu->regs.r[14] = cpu->regs.abt.r14;
            break;
            
        case CPU_STATE_UNDEFINED:
            cpu->regs.r[13] = cpu->regs.und.r13;
            cpu->regs.r[14] = cpu->regs.und.r14;
            break;

        case CPU_STATE_USER:
        case CPU_STATE_SYSTEM:
            cpu->regs.r[13] = cpu->regs.usr.r13_usr;
            cpu->regs.r[14] = cpu->regs.usr.r14_usr;
            break;
            
        default:
            break;
    }
    
    // 更新 CPSR 中的模式位
    cpu->regs.cpsr = (cpu->regs.cpsr & 0xFFFFFFE0) | new_mode;
    cpu->regs.priv_mode = new_mode;
}

/* ============================================================================
 * 中断处理
 * ============================================================================ */

void GBA_InterruptRequest(GBA_Core *core, InterruptType type) {
    core->interrupt.IF |= type;
    core->interrupt.pending |= (type & core->interrupt.IE);
    
    // 写入 I/O 寄存器
    core->memory.io_registers[0x202] = core->interrupt.IF & 0xFF;
    core->memory.io_registers[0x203] = core->interrupt.IF >> 8;
}

void GBA_InterruptClear(GBA_Core *core, InterruptType type) {
    core->interrupt.IF &= ~type;
    core->interrupt.pending &= ~type;
    
    core->memory.io_registers[0x202] = core->interrupt.IF & 0xFF;
    core->memory.io_registers[0x203] = core->interrupt.IF >> 8;
}

bool GBA_InterruptPending(const GBA_Core *core) {
    if (!core->interrupt.IME) {
        return false;
    }
    
    // 检查 CPSR 中的 IRQ 禁用位
    if (core->cpu.regs.cpsr & (1 << 7)) {
        return false;
    }
    
    return (core->interrupt.IE & core->interrupt.IF) != 0;
}

InterruptType GBA_InterruptGetHighestPriority(const GBA_Core *core) {
    uint16_t pending = core->interrupt.IE & core->interrupt.IF;
    
    // 按优先级顺序检查
    if (pending & IRQ_VBLANK)   return IRQ_VBLANK;
    if (pending & IRQ_HBLANK)   return IRQ_HBLANK;
    if (pending & IRQ_VCOUNT)   return IRQ_VCOUNT;
    if (pending & IRQ_TIMER0)   return IRQ_TIMER0;
    if (pending & IRQ_TIMER1)   return IRQ_TIMER1;
    if (pending & IRQ_TIMER2)   return IRQ_TIMER2;
    if (pending & IRQ_TIMER3)   return IRQ_TIMER3;
    if (pending & IRQ_SERIAL)   return IRQ_SERIAL;
    if (pending & IRQ_DMA0)     return IRQ_DMA0;
    if (pending & IRQ_DMA1)     return IRQ_DMA1;
    if (pending & IRQ_DMA2)     return IRQ_DMA2;
    if (pending & IRQ_DMA3)     return IRQ_DMA3;
    if (pending & IRQ_KEYPAD)   return IRQ_KEYPAD;
    if (pending & IRQ_GAMEPAK)  return IRQ_GAMEPAK;
    
    return 0;
}

void GBA_CPUCheckInterrupts(GBA_Core *core) {
    GBA_CPU *cpu = &core->cpu;

    if (cpu->state.fiq_line && !(cpu->regs.cpsr & (1 << 6))) {
        uint32_t return_address;

        if (cpu->regs.exec_mode == CPU_MODE_ARM) {
            return_address = cpu->regs.pc - 4;
        } else {
            return_address = cpu->regs.pc;
        }

        uint32_t old_cpsr = cpu->regs.cpsr;

        GBA_CPU_SwitchMode(core, CPU_STATE_FIQ);

        cpu->regs.fiq.r14_fiq = return_address;
        cpu->regs.fiq.spsr_fiq = old_cpsr;

        cpu->regs.cpsr |= (1 << 7) | (1 << 6);
        cpu->regs.cpsr &= ~(1 << 5);
        cpu->regs.exec_mode = CPU_MODE_ARM;

        cpu->regs.pc = 0x0000001C;
        GBA_CPUFlushPipeline(core);

        cpu->state.halted = false;
        cpu->state.fiq_line = false;
        return;
    }

    if (!GBA_InterruptPending(core)) {
        return;
    }

    // 保存当前状态
    uint32_t return_address;

    if (cpu->regs.exec_mode == CPU_MODE_ARM) {
        return_address = cpu->regs.pc - 4;  // ARM: PC + 4
    } else {
        return_address = cpu->regs.pc;      // THUMB: PC + 2
    }

    uint32_t old_cpsr = cpu->regs.cpsr;

    // 切换到 IRQ 模式
    GBA_CPU_SwitchMode(core, CPU_STATE_IRQ);

    // 保存返回地址和 CPSR
    cpu->regs.irq.r14 = return_address;
    cpu->regs.irq.spsr = old_cpsr;

    // 禁用 IRQ，切换到 ARM 模式
    cpu->regs.cpsr |= (1 << 7);   // 禁用 IRQ
    cpu->regs.cpsr &= ~(1 << 5);  // 清除 THUMB 位
    cpu->regs.exec_mode = CPU_MODE_ARM;

    // 跳转到中断向量 (0x00000018)
    cpu->regs.pc = 0x00000018;

    // 刷新流水线
    GBA_CPUFlushPipeline(core);

    // 唤醒 CPU（如果处于 HALT 状态）
    cpu->state.halted = false;
}

/* ============================================================================
 * PPU 更新
 * ============================================================================ */

void GBA_PPUUpdate(GBA_Core *core, uint32_t cycles) {
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t VISIBLE_SCANLINES = 160;
    const uint32_t TOTAL_SCANLINES = 228;
    
    core->ppu.scanline_cycles += cycles;
    
    while (core->ppu.scanline_cycles >= CYCLES_PER_SCANLINE) {
        core->ppu.scanline_cycles -= CYCLES_PER_SCANLINE;
        core->ppu.current_scanline++;
        
        // 检查 HBlank 中断
        if (core->memory.io_registers[0x004] & (1 << 4)) {  // DISPSTAT HBlank IRQ
            GBA_InterruptRequest(core, IRQ_HBLANK);
        }
        
        if (core->ppu.current_scanline == VISIBLE_SCANLINES) {
            // 进入 VBlank
            core->ppu.in_vblank = true;
            
            // 设置 DISPSTAT VBlank 标志
            core->memory.io_registers[0x004] |= (1 << 0);
            
            // VBlank 中断
            if (core->memory.io_registers[0x004] & (1 << 3)) {
                GBA_InterruptRequest(core, IRQ_VBLANK);
            }
            
        } else if (core->ppu.current_scanline >= TOTAL_SCANLINES) {
            // 新的一帧
            core->ppu.current_scanline = 0;
            core->ppu.in_vblank = false;
            
            // 清除 DISPSTAT VBlank 标志
            core->memory.io_registers[0x004] &= ~(1 << 0);
        }
        
        // 更新 VCOUNT 寄存器
        core->memory.io_registers[0x006] = core->ppu.current_scanline & 0xFF;
        
        // 检查 VCOUNT 匹配中断
        uint8_t vcount_setting = core->memory.io_registers[0x005];
        if (core->ppu.current_scanline == vcount_setting) {
            if (core->memory.io_registers[0x004] & (1 << 5)) {
                GBA_InterruptRequest(core, IRQ_VCOUNT);
            }
        }
    }
}

/* ============================================================================
 * 定时器更新（简化版）
 * ============================================================================ */

void GBA_TimerUpdate(GBA_Core *core, uint32_t cycles) {
    // TODO: 实现完整的定时器逻辑
    // 目前仅为占位符
    (void)core;
    (void)cycles;
}

/* ============================================================================
 * DMA 更新（简化版）
 * ============================================================================ */

void GBA_DMAUpdate(GBA_Core *core) {
    // TODO: 实现完整的 DMA 逻辑
    (void)core;
}

bool GBA_DMAIsActive(const GBA_Core *core) {
    return core->dma.active_channel >= 0;
}

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

void GBA_PPURenderFrame(GBA_Core *core) {
    // TODO: 实现渲染逻辑
    // 目前仅填充测试图案
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 240; x++) {
            // 简单的渐变测试图案
            uint8_t r = (x * 255) / 240;
            uint8_t g = (y * 255) / 160;
            uint8_t b = 128;
            
            core->ppu.framebuffer[y * 240 + x] = 
                (0xFF << 24) | (b << 16) | (g << 8) | r;
        }
    }
}
