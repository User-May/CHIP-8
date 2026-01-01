#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"

// CHIP-8内置字体集 (0-F, 每个字符5字节)
static const uint8_t FONTSET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// 初始化CHIP-8系统
void chip8_init(Chip8* chip8) {
    // 参数检查
    if (!chip8) {
        fprintf(stderr, "错误: chip8_init 参数为空\n");
        return;
    }
    
    // 清空内存
    memset(chip8->memory, 0, MEMORY_SIZE);
    
    // 清空寄存器
    memset(chip8->V, 0, sizeof(chip8->V));
    chip8->I = 0;
    chip8->pc = PROGRAM_START;
    
    // 清空堆栈
    memset(chip8->stack, 0, sizeof(chip8->stack));
    chip8->sp = 0;
    
    // 初始化定时器
    chip8->delay_timer = 0;
    chip8->sound_timer = 0;
    
    // 清空显示
    memset(chip8->display, 0, sizeof(chip8->display));
    
    // 清空键盘状态
    memset(chip8->key, 0, sizeof(chip8->key));
    
    // 初始化状态标志
    chip8->draw_flag = 1;  // 初始需要绘制
    chip8->key_wait = 0;
    chip8->key_reg = 0;
    
    // 加载字体集到内存 0x000-0x04F 区域
    for (int i = 0; i < 80; i++) {
        chip8->memory[i] = FONTSET[i];
    }
    
    printf("CHIP-8 系统初始化完成\n");
    printf("内存: 4KB, 显示: %dx%d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    printf("程序起始地址: 0x%03X\n", PROGRAM_START);
    printf("字体集加载到: 0x000-0x04F\n");
}

// 加载ROM文件
int chip8_load_rom(Chip8* chip8, const char* filename) {
    if (!chip8 || !filename) {
        fprintf(stderr, "错误: chip8_load_rom 参数为空\n");
        return 0;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "错误: 无法打开ROM文件: %s\n", filename);
        return 0;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // 检查文件大小
    long max_size = MEMORY_SIZE - PROGRAM_START;
    if (file_size > max_size) {
        fprintf(stderr, "错误: ROM文件太大 (%ld字节 > %ld字节可用)\n", 
                file_size, max_size);
        fclose(file);
        return 0;
    }
    
    // 读取ROM到内存
    size_t bytes_read = fread(&chip8->memory[PROGRAM_START], 
                              sizeof(uint8_t), 
                              file_size, 
                              file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "错误: 读取ROM不完整 (读取 %zu字节，预期 %ld字节)\n", 
                bytes_read, file_size);
        return 0;
    }
    
    printf("成功加载ROM: %s\n", filename);
    printf("文件大小: %ld字节\n", file_size);
    printf("加载到内存地址: 0x%03X-0x%03X\n", 
           PROGRAM_START, 
           PROGRAM_START + (uint16_t)file_size - 1);
    
    return 1;  // 成功
}

// CPU单周期执行
void chip8_cycle(Chip8* chip8) {
    if (!chip8) return;

    // 1. 取指 (Fetch): 从当前PC位置读取一个16位的操作码
    // CHIP-8的操作码是大端序，高字节在前
    uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    
    // 调试：打印当前执行信息
    printf("[执行] PC=0x%03X, Opcode=0x%04X\n", chip8->pc, opcode);

    // 2. 解码与执行 (Decode & Execute)
    // 先通过操作码的高位（第一个16进制数字）判断指令类型
    switch (opcode & 0xF000) { // 掩码 0xF000 用于取高4位
        case 0x0000:
            // 以0开头的指令 (如 00E0, 00EE)
            switch (opcode) {
                case 0x00E0: // 00E0: 清屏 (CLS)
                    printf("  执行: 00E0 (清屏)\n");
                    // 清空显示缓冲区
                    memset(chip8->display, 0, sizeof(chip8->display));
                    chip8->draw_flag = 1; // 标记需要更新屏幕
                    chip8->pc += 2; // PC前进
                    break;
                case 0x00EE: // 00EE: 从子程序返回 (RET)
                    printf("  执行: 00EE (返回)\n");
                    // 后续实现堆栈逻辑
                    chip8->pc += 2;
                    break;
                default:
                    printf("  未知的0指令: 0x%04X\n", opcode);
                    chip8->pc += 2;
            }
            break;

        case 0x1000: // 1NNN: 跳转到地址 NNN (JP NNN)
            {
                uint16_t address = opcode & 0x0FFF; // 取后12位作为地址
                printf("  执行: 1NNN (跳转到 0x%03X)\n", address);
                chip8->pc = address; // 直接设置PC，注意这里不加2
            }
            break;

        case 0x6000: // 6XNN: 将常数NN存入寄存器VX (LD Vx, byte)
            {
                uint8_t x = (opcode & 0x0F00) >> 8; // 取寄存器索引
                uint8_t nn = opcode & 0x00FF; // 取常数
                printf("  执行: 6XNN (设置 V%X = 0x%02X)\n", x, nn);
                chip8->V[x] = nn;
                chip8->pc += 2;
            }
            break;

        case 0x8000: // 8XY4: 将VY加到VX，VF作为进位标志 (ADD Vx, Vy)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                printf("  执行: 8XY4 (V%X = V%X + V%X)\n", x, x, y);
                
                uint16_t sum = chip8->V[x] + chip8->V[y];
                chip8->V[0xF] = (sum > 0xFF) ? 1 : 0; // 设置进位标志
                chip8->V[x] = sum & 0xFF; // 保留低8位
                chip8->pc += 2;
            }
            break;

        default:
            printf("  尚未实现的操作码: 0x%04X\n", opcode);
            chip8->pc += 2; // 即使未实现，也前进PC以避免死循环
            break;
    }
}

// 更新定时器 (每秒60次调用.)
void chip8_update_timers(Chip8* chip8) {
    if (chip8->delay_timer > 0) {
        chip8->delay_timer--;
    }
    
    if (chip8->sound_timer > 0) {
        if (chip8->sound_timer == 1) {
            printf("BEEP! 播放提示音\n");
        }
        chip8->sound_timer--;
    }
}