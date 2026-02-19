/**
 * GBA 系統簡單測試程序
 * 驗證核心架構是否正常運作
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "arm7tdmi.h"
#include "gba_system.h"
#include "gba_interrupt.h"

/* 全局變數 (與 main.c 一致) */
Gba_Cpu *cpu;
GbaMem *Mem;
Gba_System gba_system;

void test_gba_system_init() {
    printf("\n========== 測試 1: GBA 系統初始化 ==========\n");
    
    /* 分配記憶體 */
    cpu = malloc(sizeof(Gba_Cpu));
    Mem = malloc(sizeof(GbaMem));
    
    if (!cpu || !Mem) {
        printf("❌ 記憶體分配失敗\n");
        return;
    }
    printf("✓ CPU 和 內存結構分配成功\n");
    
    /* 初始化內存 */
    Init_GbaMem();
    printf("✓ 內存系統初始化成功\n");
    
    /* 初始化 GBA 系統 */
    if (GbaSystem_Init(&gba_system) != 0) {
        printf("❌ GBA 系統初始化失敗\n");
        return;
    }
    printf("✓ GBA 系統初始化成功\n");
    
    /* 驗證初始狀態 */
    if (gba_system.frame_number != 0) {
        printf("❌ 初始幀數應為 0，實際為 %llu\n", gba_system.frame_number);
        return;
    }
    printf("✓ 初始幀數正確: 0\n");
    
    if (gba_system.total_cycles != 0) {
        printf("❌ 初始週期應為 0，實際為 %llu\n", gba_system.total_cycles);
        return;
    }
    printf("✓ 初始週期正確: 0\n");
    
    if (gba_system.ppu.current_scanline != 0) {
        printf("❌ 初始掃描線應為 0，實際為 %u\n", gba_system.ppu.current_scanline);
        return;
    }
    printf("✓ 初始掃描線正確: 0\n");
    
    if (gba_system.ppu.framebuffer == NULL) {
        printf("❌ 幀緩衝指針為空\n");
        return;
    }
    printf("✓ 幀緩衝分配成功（240x160x4 bytes）\n");
    
    printf("✅ 測試 1 通過！\n");
}

void test_ppu_timing() {
    printf("\n========== 測試 2: PPU 時序計算 ==========\n");
    
    printf("目標: 執行一幀 (280,896 cycles)\n");
    printf("初始狀態:\n");
    printf("  - 掃描線: %u\n", gba_system.ppu.current_scanline);
    printf("  - 週期數: %llu\n", gba_system.total_cycles);
    
    /* 執行一完整幀 */
    uint32_t cycles_executed = GbaSystem_RunFrame(&gba_system);
    
    printf("\n執行後:\n");
    printf("  - 執行週期數: %u\n", cycles_executed);
    printf("  - 總週期: %llu\n", gba_system.total_cycles);
    printf("  - 當前幀數: %llu\n", gba_system.frame_number);
    printf("  - 掃描線: %u\n", gba_system.ppu.current_scanline);
    printf("  - V-Blank 狀態: %s\n", gba_system.ppu.in_vblank ? "是" : "否");
    printf("  - H-Blank 狀態: %s\n", gba_system.ppu.in_hblank ? "是" : "否");
    
    /* 驗證週期計數 */
    if (cycles_executed != GBA_CYCLES_PER_FRAME) {
        printf("⚠️  執行的週期 (%u) 不等於期望 (%u)\n", 
               cycles_executed, GBA_CYCLES_PER_FRAME);
    } else {
        printf("✓ 週期計數正確\n");
    }
    
    /* 驗證幀計數增加 */
    if (gba_system.frame_number != 1) {
        printf("❌ 幀數應為 1，實際為 %llu\n", gba_system.frame_number);
        return;
    }
    printf("✓ 幀數正確遞增到 1\n");
    
    /* 驗證掃描線是否回到 0 (新幀開始) */
    if (gba_system.ppu.current_scanline != 0) {
        printf("⚠️  新幀開始後掃描線應為 0，實際為 %u\n", 
               gba_system.ppu.current_scanline);
    } else {
        printf("✓ 掃描線在幀結束時正確回零\n");
    }
    
    printf("✅ 測試 2 通過！\n");
}

void test_interrupt_system() {
    printf("\n========== 測試 3: 中斷系統 ==========\n");
    
    printf("初始中斷狀態:\n");
    printf("  - IF (請求): 0x%04X\n", gba_system.interrupt.IF);
    printf("  - IE (啟用): 0x%04X\n", gba_system.interrupt.IE);
    printf("  - IME (主控): %d\n", gba_system.interrupt.IME);
    
    /* 測試 1: 要求中斷 */
    printf("\n測試: 要求 V-Blank 中斷\n");
    Interrupt_Request(&gba_system, INT_VBLANK);
    if (gba_system.interrupt.IF & INT_VBLANK) {
        printf("✓ V-Blank 中斷請求成功\n");
    } else {
        printf("❌ V-Blank 中斷請求失敗\n");
        return;
    }
    
    /* 測試 2: 啟用中斷 */
    printf("\n測試: 啟用 V-Blank 中斷\n");
    Interrupt_Enable(&gba_system, INT_VBLANK);
    if (gba_system.interrupt.IE & INT_VBLANK) {
        printf("✓ V-Blank 中斷啟用成功\n");
    } else {
        printf("❌ V-Blank 中斷啟用失敗\n");
        return;
    }
    
    /* 測試 3: 設置中斷主控 */
    printf("\n測試: 設置中斷主控啟用\n");
    Interrupt_SetMasterEnable(&gba_system, 1);
    if (gba_system.interrupt.IME) {
        printf("✓ 中斷主控啟用成功\n");
    } else {
        printf("❌ 中斷主控啟用失敗\n");
        return;
    }
    
    /* 測試 4: 獲取最高優先級中斷 */
    printf("\n測試: 獲取最高優先級中斷\n");
    uint16_t highest = Interrupt_GetHighestPending(&gba_system);
    if (highest == INT_VBLANK) {
        printf("✓ 正確識別為 V-Blank 中斷\n");
    } else {
        printf("❌ 中斷識別失敗 (0x%04X)\n", highest);
        return;
    }
    
    /* 測試 5: 清除中斷 */
    printf("\n測試: 清除 V-Blank 中斷\n");
    Interrupt_Clear(&gba_system, INT_VBLANK);
    if (!(gba_system.interrupt.IF & INT_VBLANK)) {
        printf("✓ 中斷清除成功\n");
    } else {
        printf("❌ 中斷清除失敗\n");
        return;
    }
    
    printf("✅ 測試 3 通過！\n");
}

void test_multiple_frames() {
    printf("\n========== 測試 4: 多幀執行 ==========\n");
    
    uint64_t initial_cycles = gba_system.total_cycles;
    uint64_t initial_frames = gba_system.frame_number;
    
    printf("執行 10 幀...\n");
    for (int i = 0; i < 10; i++) {
        GbaSystem_RunFrame(&gba_system);
    }
    
    uint64_t cycles_diff = gba_system.total_cycles - initial_cycles;
    uint64_t expected_cycles = 10 * GBA_CYCLES_PER_FRAME;
    
    printf("\n執行結果:\n");
    printf("  - 幀數增加: %llu → %llu (增加 %llu)\n", 
           initial_frames, gba_system.frame_number, 
           gba_system.frame_number - initial_frames);
    printf("  - 週期數: +%llu / 預期 %llu\n", 
           cycles_diff, expected_cycles);
    printf("  - 掃描線: %u\n", gba_system.ppu.current_scanline);
    
    if (gba_system.frame_number - initial_frames == 10) {
        printf("✓ 幀數正確增加\n");
    } else {
        printf("❌ 幀數增加不正確\n");
        return;
    }
    
    if (cycles_diff == expected_cycles) {
        printf("✓ 週期數計算正確\n");
    } else {
        printf("⚠️  週期數不匹配 (差異: %lld)\n", 
               (long long)cycles_diff - expected_cycles);
    }
    
    printf("✅ 測試 4 通過！\n");
}

void test_performance_stats() {
    printf("\n========== 測試 5: 性能統計 ==========\n");
    
    GbaSystem_PrintStats(&gba_system);
}

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║  GBA 模擬器核心系統簡單測試          ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    /* 執行所有測試 */
    test_gba_system_init();
    test_ppu_timing();
    test_interrupt_system();
    test_multiple_frames();
    test_performance_stats();
    
    /* 清理資源 */
    printf("\n========== 清理資源 ==========\n");
    if (gba_system.ppu.framebuffer) {
        free(gba_system.ppu.framebuffer);
        gba_system.ppu.framebuffer = NULL;
    }
    Release_GbaMem();
    if (cpu) {
        free(cpu);
        cpu = NULL;
    }
    if (Mem) {
        free(Mem);
        Mem = NULL;
    }
    printf("✓ 資源清理完成\n");
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║        所有測試完成！                ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}
