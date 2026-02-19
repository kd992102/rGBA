/**
 * GBA模拟器主程序 - 新架构版本
 * 使用统一的 GBA_Core 接口
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "gba_core.h"

/* 配置 */
static const char *BIOS_FILE = "gba_bios.bin";
static const char *ROM_FILE = "suite.gba";
static const int ENABLE_SDL = 0;
static const uint32_t MAX_FRAMES_NO_SDL = 10000;
static const uint32_t REPORT_INTERVAL_FRAMES = 60;
static const uint32_t DUMP_INSTRUCTIONS = 100;
static const uint32_t SKIP_INSTRUCTIONS = 0;
static const int USE_MGBA_BIOS_STATE = 1;

static uint16_t ReadIO16(const GBA_Core *gba, uint32_t address) {
    uint32_t offset = address & 0x3FF;
    const uint8_t *io = gba->memory.io_registers;
    return (uint16_t)(io[offset] | (io[offset + 1] << 8));
}

static void WriteIO16(GBA_Core *gba, uint32_t address, uint16_t value) {
    uint32_t offset = address & 0x3FF;
    uint8_t *io = gba->memory.io_registers;
    io[offset] = (uint8_t)(value & 0xFF);
    io[offset + 1] = (uint8_t)(value >> 8);
}

static void InitMinimalIO(GBA_Core *gba) {
    WriteIO16(gba, 0x04000000, 0x0080); // DISPCNT (forced blank after RegisterRamReset)
    WriteIO16(gba, 0x04000004, 0x0000); // DISPSTAT
    WriteIO16(gba, 0x04000006, 0x0000); // VCOUNT
    WriteIO16(gba, 0x04000200, 0x0000); // IE
    WriteIO16(gba, 0x04000202, 0x0000); // IF
    WriteIO16(gba, 0x04000208, 0x0000); // IME
    WriteIO16(gba, 0x04000100, 0x0000); // TM0CNT_L
    WriteIO16(gba, 0x04000102, 0x0000); // TM0CNT_H
    WriteIO16(gba, 0x04000104, 0x0000); // TM1CNT_L
    WriteIO16(gba, 0x04000106, 0x0000); // TM1CNT_H
    WriteIO16(gba, 0x04000108, 0x0000); // TM2CNT_L
    WriteIO16(gba, 0x0400010A, 0x0000); // TM2CNT_H
    WriteIO16(gba, 0x0400010C, 0x0000); // TM3CNT_L
    WriteIO16(gba, 0x0400010E, 0x0000); // TM3CNT_H
    WriteIO16(gba, 0x04000204, 0x0000); // WAITCNT (reset default)
    WriteIO16(gba, 0x04000300, 0x0001); // POSTFLG (set by BIOS after RegisterRamReset)
}

static void ApplyMgbaBiosState(GBA_Core *gba) {
    GBA_CPU *cpu = &gba->cpu;

    memset(cpu->regs.r, 0, sizeof(cpu->regs.r));
    cpu->regs.r[13] = 0x03007F00; // SP
    cpu->regs.r[14] = 0x00000000; // LR
    cpu->regs.exec_addr = 0x00000000; // 執行地址
    cpu->regs.pc = 0x00000008;        // PC = exec_addr + 8 (ARM)
    cpu->regs.cpsr = 0x0000001F; // System, ARM, IRQ/FIQ enabled
    cpu->regs.exec_mode = CPU_MODE_ARM;
    cpu->regs.priv_mode = CPU_STATE_SYSTEM;
    cpu->state.halted = false;

    cpu->regs.usr.r8_usr = cpu->regs.r[8];
    cpu->regs.usr.r9_usr = cpu->regs.r[9];
    cpu->regs.usr.r10_usr = cpu->regs.r[10];
    cpu->regs.usr.r11_usr = cpu->regs.r[11];
    cpu->regs.usr.r12_usr = cpu->regs.r[12];
    cpu->regs.usr.r13_usr = cpu->regs.r[13];
    cpu->regs.usr.r14_usr = cpu->regs.r[14];
    
    // 清除所有 IO 寄存器（BIOS 入口状態下未初始化）
    memset(gba->memory.io_registers, 0, 0x400);

    GBA_CPUFlushPipeline(gba);
}

static void EnableInterrupts(GBA_Core *gba) {
    gba->interrupt.IE = (uint16_t)(IRQ_VBLANK | IRQ_TIMER0);
    gba->interrupt.IME = 1;
    WriteIO16(gba, 0x04000200, gba->interrupt.IE);
    WriteIO16(gba, 0x04000208, gba->interrupt.IME);
}

static void SimulateVBlankTimer(GBA_Core *gba, uint32_t step_cycles) {
    static uint32_t cycle_accum = 0;
    static uint16_t last_vcount = 0;
    static uint32_t timer0_accum = 0;

    cycle_accum += step_cycles;
    uint16_t vcount = (uint16_t)((cycle_accum / 1232) % 228);
    uint16_t dispstat = ReadIO16(gba, 0x04000004);

    if (vcount >= 160) {
        dispstat |= 0x0001; // VBlank
    } else {
        dispstat &= ~0x0001;
    }

    if (vcount == 160 && last_vcount != 160) {
        GBA_InterruptRequest(gba, IRQ_VBLANK);
    }

    WriteIO16(gba, 0x04000004, dispstat);
    WriteIO16(gba, 0x04000006, vcount);
    gba->ppu.DISPSTAT = dispstat;
    gba->ppu.VCOUNT = vcount;
    last_vcount = vcount;

    timer0_accum += step_cycles;
    if (timer0_accum >= 1024) {
        timer0_accum -= 1024;
        GBA_InterruptRequest(gba, IRQ_TIMER0);
    }
}

static void DumpIOSnapshot(const GBA_Core *gba) {
    printf("[IO] DISPSTAT: 0x%04X | VCOUNT: 0x%04X\n",
        ReadIO16(gba, 0x04000004), ReadIO16(gba, 0x04000006));
    printf("[IO] IF: 0x%04X | IE: 0x%04X | IME: 0x%04X\n",
        ReadIO16(gba, 0x04000202), ReadIO16(gba, 0x04000200), ReadIO16(gba, 0x04000208));
    printf("[IO] TM0: 0x%04X/0x%04X | TM1: 0x%04X/0x%04X\n",
        ReadIO16(gba, 0x04000100), ReadIO16(gba, 0x04000102),
        ReadIO16(gba, 0x04000104), ReadIO16(gba, 0x04000106));
    printf("[IO] TM2: 0x%04X/0x%04X | TM3: 0x%04X/0x%04X\n",
        ReadIO16(gba, 0x04000108), ReadIO16(gba, 0x0400010A),
        ReadIO16(gba, 0x0400010C), ReadIO16(gba, 0x0400010E));
    printf("[IO] WAITCNT: 0x%04X\n", ReadIO16(gba, 0x04000204));
}

static void DumpInstructions(GBA_Core *gba, uint32_t skip, uint32_t count) {
    for (uint32_t i = 0; i < skip; i++) {
        GBA_CPUStep(gba);
        SimulateVBlankTimer(gba, 4);
    }

    for (uint32_t i = 0; i < count; i++) {
        uint32_t exec_addr = gba->cpu.regs.exec_addr;
        uint32_t opcode = gba->cpu.regs.exec_mode == CPU_MODE_ARM
                              ? gba->cpu.regs.pipeline[0]
                              : (gba->cpu.regs.pipeline[0] & 0xFFFF);
        const char *mode = (gba->cpu.regs.exec_mode == CPU_MODE_ARM) ? "ARM" : "THUMB";

        // 執行指令
        GBA_CPUStep(gba);
        SimulateVBlankTimer(gba, 4);
        
        // 顯示執行後的狀態（與 mGBA 一致）
        uint32_t pc = gba->cpu.regs.pc;
        uint32_t cpsr = gba->cpu.regs.cpsr;

        printf("[inst %03u] EXEC: 0x%08X | OPC: 0x%08X | %s\n",
            skip + i + 1, exec_addr, opcode, mode);
        
        // 打印寄存器狀態（與 mGBA 格式相同）
        printf(" r0: %08X   r1: %08X   r2: %08X   r3: %08X\n",
            gba->cpu.regs.r[0], gba->cpu.regs.r[1], gba->cpu.regs.r[2], gba->cpu.regs.r[3]);
        printf(" r4: %08X   r5: %08X   r6: %08X   r7: %08X\n",
            gba->cpu.regs.r[4], gba->cpu.regs.r[5], gba->cpu.regs.r[6], gba->cpu.regs.r[7]);
        printf(" r8: %08X   r9: %08X  r10: %08X  r11: %08X\n",
            gba->cpu.regs.r[8], gba->cpu.regs.r[9], gba->cpu.regs.r[10], gba->cpu.regs.r[11]);
        printf("r12: %08X  r13: %08X  r14: %08X  r15: %08X\n",
            gba->cpu.regs.r[12], gba->cpu.regs.r[13], gba->cpu.regs.r[14], pc);
        
        // 打印 CPSR 和標誌位
        bool N = (cpsr >> 31) & 1;
        bool Z = (cpsr >> 30) & 1;
        bool C = (cpsr >> 29) & 1;
        bool V = (cpsr >> 28) & 1;
        bool I = (cpsr >> 7) & 1;   // IRQ disable
        bool F = (cpsr >> 6) & 1;   // FIQ disable
        bool T = (cpsr >> 5) & 1;   // THUMB mode
        printf("cpsr: %08X [%c%c%c%c%c%c%c]\n\n", cpsr,
            N ? 'N' : '-', Z ? 'Z' : '-', C ? 'C' : '-', V ? 'V' : '-',
            I ? 'I' : '-', F ? 'F' : '-', T ? 'T' : '-');
    }

    DumpIOSnapshot(gba);
}

/* SDL 全局变量 */
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

/* ============================================================================
 * 文件加载utility
 * ============================================================================ */

uint8_t* LoadFile(const char *filename, size_t *out_size) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "[错误] 无法打开文件: %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t *data = (uint8_t*)malloc(size);
    if (!data) {
        fprintf(stderr, "[错误] 内存分配失败: %zu bytes\n", size);
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(data, 1, size, file);
    fclose(file);
    
    if (read_size != size) {
        fprintf(stderr, "[错误] 文件读取不完整: %zu / %zu bytes\n", read_size, size);
        free(data);
        return NULL;
    }
    
    *out_size = size;
    return data;
}

/* ============================================================================
 * SDL 初始化
 * ============================================================================ */

int InitSDL() {
    printf("[SDL] 初始化 SDL...\n");
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "[错误] SDL 初始化失败: %s\n", SDL_GetError());
        return -1;
    }
    
    // 创建窗口（2倍放大）
    window = SDL_CreateWindow(
        "rGBA Emulator - New Architecture",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        240 * 2, 160 * 2,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        fprintf(stderr, "[错误] 无法创建窗口: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    
    // 创建渲染器
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "[错误] 无法创建渲染器: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // 创建纹理
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ABGR8888,  // GBA 使用 ABGR
        SDL_TEXTUREACCESS_STREAMING,
        240, 160
    );
    
    if (!texture) {
        fprintf(stderr, "[错误] 无法创建纹理: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    
    printf("[SDL] 初始化完成\n");
    return 0;
}

void CleanupSDL() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}

/* ============================================================================
 * 主程序
 * ============================================================================ */

int main(int argc, char *argv[]) {
    printf("========== rGBA 模拟器（新架构）==========\n");
    printf("架构版本: 现代化设计\n");
    printf("特性: 无全局变量、统一接口、高性能\n");
    printf("=========================================\n\n");
    
    /* === 第一阶段：创建GBA核心 === */
    printf("[初始化] 创建 GBA Core...\n");
    GBA_Core *gba = GBA_CoreCreate();
    if (!gba) {
        fprintf(stderr, "[错误] GBA Core 创建失败\n");
        return 1;
    }
    
    /* === 第二阶段：加载BIOS === */
    printf("[初始化] 加载 BIOS: %s\n", BIOS_FILE);
    size_t bios_size = 0;
    uint8_t *bios_data = LoadFile(BIOS_FILE, &bios_size);
    
    if (!bios_data) {
        fprintf(stderr, "[错误] 无法加载 BIOS\n");
        GBA_CoreDestroy(gba);
        return 1;
    }
    
    if (GBA_CoreLoadBIOS(gba, bios_data, bios_size) != 0) {
        fprintf(stderr, "[错误] BIOS 加载失败\n");
        free(bios_data);
        GBA_CoreDestroy(gba);
        return 1;
    }
    free(bios_data);
    
    /* === 第三阶段：加载ROM === */
    printf("[初始化] 加载 ROM: %s\n", ROM_FILE);
    size_t rom_size = 0;
    uint8_t *rom_data = LoadFile(ROM_FILE, &rom_size);
    
    if (!rom_data) {
        fprintf(stderr, "[错误] 无法加载 ROM\n");
        GBA_CoreDestroy(gba);
        return 1;
    }
    
    if (GBA_CoreLoadROM(gba, rom_data, rom_size) != 0) {
        fprintf(stderr, "[错误] ROM 加载失败\n");
        free(rom_data);
        GBA_CoreDestroy(gba);
        return 1;
    }
    free(rom_data);
    
    uint32_t frame_count = 0;
    uint64_t start_ticks = ENABLE_SDL ? SDL_GetTicks64() : 0;
    uint64_t last_stats_ticks = start_ticks;

    if (!ENABLE_SDL) {
        printf("[初始化] SDL 已禁用，僅執行指令測試模式\n\n");
        if (DUMP_INSTRUCTIONS > 0) {
            InitMinimalIO(gba);
            if (USE_MGBA_BIOS_STATE) {
                printf("[初始化] 使用 mGBA BIOS 入口初始狀態（執行 BIOS）\n");
                ApplyMgbaBiosState(gba);
            }
            EnableInterrupts(gba);
            DumpInstructions(gba, SKIP_INSTRUCTIONS, DUMP_INSTRUCTIONS);
            goto cleanup;
        }
    }

    /* === 第四阶段：初始化SDL === */
    if (ENABLE_SDL) {
        if (InitSDL() != 0) {
            GBA_CoreDestroy(gba);
            return 1;
        }
    }

    if (USE_MGBA_BIOS_STATE) {
        printf("[初始化] 使用 mGBA BIOS 入口初始狀態（執行 BIOS）\n");
        ApplyMgbaBiosState(gba);
    }
    
    /* === 第五阶段：主循环 === */
    printf("\n[运行] 进入主循环...\n\n");
    
    bool running = true;
    
    while (running) {
        if (ENABLE_SDL) {
            /* 处理SDL事件 */
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT:
                        running = false;
                        printf("\n[事件] 关闭窗口\n");
                        break;
                        
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            running = false;
                            printf("\n[事件] 按下 ESC 键\n");
                        } else if (event.key.keysym.sym == SDLK_SPACE) {
                            printf("\n");
                            GBA_CoreDumpState(gba);
                        }
                        break;
                }
            }
        }
        
        /* 执行一帧 (280,896 周期) */
        uint32_t cycles_executed = GBA_CoreRunFrame(gba);
        frame_count++;
        
        if (ENABLE_SDL) {
            /* 获取帧缓冲 */
            const uint32_t *framebuffer = GBA_CoreGetFramebuffer(gba);
            
            /* 更新SDL纹理 */
            void *pixels;
            int pitch;
            if (SDL_LockTexture(texture, NULL, &pixels, &pitch) == 0) {
                memcpy(pixels, framebuffer, 240 * 160 * sizeof(uint32_t));
                SDL_UnlockTexture(texture);
            }
            
            /* 渲染 */
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        
        if (frame_count % REPORT_INTERVAL_FRAMES == 0) {
            if (ENABLE_SDL) {
                uint64_t current_ticks = SDL_GetTicks64();
                if (current_ticks - last_stats_ticks >= 1000) {
                    uint64_t elapsed = current_ticks - start_ticks;
                    float seconds = elapsed / 1000.0f;
                    float fps = frame_count / seconds;
                    
                    printf("[性能] 帧数: %u | FPS: %.2f | 总周期: %llu | 本幀周期: %u\n",
                           frame_count, fps,
                           (unsigned long long)gba->cpu.cycles.total,
                           cycles_executed);
                    
                    last_stats_ticks = current_ticks;
                }
            }
            printf("[状态] 帧数: %u | PC: 0x%08X | CPSR: 0x%08X | 模式: %s/%s\n",
                   frame_count,
                   gba->cpu.regs.pc,
                   gba->cpu.regs.cpsr,
                   gba->cpu.regs.exec_mode == CPU_MODE_ARM ? "ARM" : "THUMB",
                   gba->cpu.regs.priv_mode == CPU_STATE_SYSTEM ? "SYS" : "PRIV");
        }

        if (!ENABLE_SDL && frame_count >= MAX_FRAMES_NO_SDL) {
            running = false;
        }
        
        /* 帧率限制（可选，避免CPU占用过高）*/
        // SDL_Delay(1); // 大约限制到 1000 FPS
    }
    
cleanup:
    /* === 第六阶段：清理 === */
    printf("\n[清理] 释放资源...\n");
    
    printf("\n========== 最终统计 ==========\n");
    printf("总帧数: %u\n", frame_count);
    printf("总周期: %llu\n", (unsigned long long)gba->cpu.cycles.total);
    if (ENABLE_SDL) {
        printf("平均 FPS: %.2f\n", 
               frame_count / ((SDL_GetTicks64() - start_ticks) / 1000.0f));
    }
    printf("===============================\n\n");
    
    if (ENABLE_SDL) {
        CleanupSDL();
    }
    GBA_CoreDestroy(gba);
    
    printf("[完成] 模拟器已关闭\n");
    return 0;
}
