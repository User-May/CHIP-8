#include <stdio.h>
#include <string.h>
#include "chip8.h"

// 显示内存内容（调试用）
void print_memory(Chip8* chip8, uint16_t start, uint16_t length) {
    printf("内存内容 (0x%03X-0x%03X):\n", start, start + length - 1);
    
    for (uint16_t i = 0; i < length; i += 16) {
        printf("0x%03X: ", start + i);
        for (int j = 0; j < 16 && (i + j) < length; j++) {
            printf("%02X ", chip8->memory[start + i + j]);
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    printf("=== CHIP-8 模拟器 ===\n");
    printf("构建日期: %s %s\n", __DATE__, __TIME__);
    
    // 创建并初始化CPU
    Chip8 chip8;
    chip8_init(&chip8);
    
    // 加载ROM
    const char* rom_filename = "test_real.ch8"; // 默认使用正确的二进制ROM
    if (argc > 1) {
        rom_filename = argv[1];  // 使用命令行参数指定的文件
    }
    
    if (chip8_load_rom(&chip8, rom_filename)) {
        printf("\nROM加载成功！\n");
        
        // 显示ROM的前48字节
        print_memory(&chip8, PROGRAM_START, 48);
        
        // 显示字体集（可选）
        // printf("\n字体集 (0x000-0x04F):\n");
        // print_memory(&chip8, 0x000, 0x50);
        
        // ========== 核心模拟循环 ==========
        printf("\n=== 开始模拟执行 ===\n");
        
        int cycles_to_run = 5; // 设置要执行的周期数
        printf("将执行 %d 个CPU周期...\n\n", cycles_to_run);
        
        for (int i = 0; i < cycles_to_run; i++) {
            printf("[周期 %d] ", i + 1);
            chip8_cycle(&chip8);
            
            // 可选：显示寄存器状态（调试用）
            if (i < 4) { // 只显示前几次周期的寄存器变化
                printf("    寄存器状态 -> ");
                printf("V1=0x%02X, V2=0x%02X, VF=0x%02X", 
                       chip8.V[1], chip8.V[2], chip8.V[0xF]);
                printf(", PC=0x%03X\n", chip8.pc);
            }
        }
        
        printf("\n=== 模拟执行完成 ===\n");
        
        // 最终寄存器状态报告
        printf("\n最终寄存器状态:\n");
        for (int i = 0; i < 16; i++) {
            if (chip8.V[i] != 0 || i == 0xF) { // 显示非零寄存器或标志寄存器
                printf("  V%X = 0x%02X (%3d)", i, chip8.V[i], chip8.V[i]);
                if (i == 1) printf(" (V1 = 2 + 3 = 5)");
                if (i == 0xF) printf(" (进位标志)");
                printf("\n");
            }
        }
        printf("  PC = 0x%03X\n", chip8.pc);
        printf("  I  = 0x%03X\n", chip8.I);
        
    } else {
        // ROM加载失败的回退方案
        printf("\nROM加载失败，使用内置示例程序...\n");
        
        // 创建一个简单的测试程序
        chip8.memory[PROGRAM_START]     = 0x00;
        chip8.memory[PROGRAM_START + 1] = 0xE0;  // 00E0: 清屏
        chip8.memory[PROGRAM_START + 2] = 0x61;  // 6102: LD V1, 0x02
        chip8.memory[PROGRAM_START + 3] = 0x02;
        chip8.memory[PROGRAM_START + 4] = 0x62;  // 6203: LD V2, 0x03
        chip8.memory[PROGRAM_START + 5] = 0x03;
        chip8.memory[PROGRAM_START + 6] = 0x81;  // 8124: ADD V1, V2
        chip8.memory[PROGRAM_START + 7] = 0x24;
        chip8.memory[PROGRAM_START + 8] = 0x12;  // 1200: JP 0x200
        chip8.memory[PROGRAM_START + 9] = 0x00;
        
        printf("已加载内置测试程序 (10字节)\n");
        
        // 执行几个周期
        printf("\n执行 3 个CPU周期...\n");
        for (int i = 0; i < 3; i++) {
            printf("[周期 %d] ", i + 1);
            chip8_cycle(&chip8);
        }
    }
    
    printf("\n\n按回车键退出...");
    fflush(stdout);
    
    // 清除输入缓冲区，确保能捕获回车键
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
    
    return 0;
}