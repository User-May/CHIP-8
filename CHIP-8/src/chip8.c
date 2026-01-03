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
    size_t bytes_read = fread(&chip8->memory[PROGRAM_START],sizeof(uint8_t),file_size,file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr,"错误: 读取ROM不完整 (读取 %zu字节，预期 %ld字节)\n",bytes_read, file_size);
        return 0;
    }
    
    printf("成功加载ROM: %s\n", filename);
    printf("文件大小: %ld字节\n", file_size);
    printf("加载到内存地址: 0x%03X-0x%03X\n",PROGRAM_START, PROGRAM_START + (uint16_t)file_size - 1);
    return 1;  // 成功
}

// CPU单周期执行：取指、解码、执行
void chip8_cycle(Chip8* chip8) {
    if (!chip8) return;

    // 1. 取指 (Fetch): 从当前PC位置读取一个16位的操作码
    uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    
    // 调试：打印当前执行信息
    printf("[执行] PC=0x%03X, Opcode=0x%04X\n", chip8->pc, opcode);

    // 2. 解码与执行
    switch (opcode & 0xF000) {
        // ============ 0xxx: 特殊指令 ============
        case 0x0000:
            switch (opcode) {
                case 0x00E0: // 00E0: 清屏 (CLS)
                    printf("  执行: 00E0 (清屏)\n");
                    memset(chip8->display, 0, sizeof(chip8->display));
                    chip8->draw_flag = 1;
                    chip8->pc += 2;
                    break;
                    
                case 0x00EE: // 00EE: 从子程序返回 (RET)
                    printf("  执行: 00EE (返回)\n");
                    // 堆栈逻辑：从堆栈弹出地址
                    if (chip8->sp > 0) {
                        chip8->sp--;
                        chip8->pc = chip8->stack[chip8->sp];
                    } else {
                        printf("  警告: 堆栈下溢!\n");
                        chip8->pc += 2;
                    }
                    break;
                    
                default:
                    // SYS addr - 现代模拟器通常忽略
                    printf("  执行: SYS 0x%03X (忽略)\n", opcode & 0x0FFF);
                    chip8->pc += 2;
                    break;
            }
            break;

        // ============ 1xxx: 跳转 ============
        case 0x1000: // 1NNN: 跳转到地址 NNN (JP NNN)
            {
                uint16_t address = opcode & 0x0FFF;
                printf("  执行: 1NNN (跳转到 0x%03X)\n", address);
                chip8->pc = address; // 直接设置PC，不加2
            }
            break;

        // ============ 2xxx: 调用子程序 ============
        case 0x2000: // 2NNN: 调用子程序 (CALL NNN)
            {
                uint16_t address = opcode & 0x0FFF;
                printf("  执行: 2NNN (调用子程序 0x%03X)\n", address);
                
                // 将返回地址压栈
                if (chip8->sp < 16) {
                    chip8->stack[chip8->sp] = chip8->pc + 2;
                    chip8->sp++;
                    chip8->pc = address;
                } else {
                    printf("  警告: 堆栈溢出!\n");
                    chip8->pc += 2;
                }
            }
            break;

        // ============ 3xxx: 条件跳过 ============
        case 0x3000: // 3XNN: 如果 VX == NN 则跳过 (SE Vx, byte)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t nn = opcode & 0x00FF;
                printf("  执行: 3XNN (如果 V%X==0x%02X 则跳过)\n", x, nn);
                
                if (chip8->V[x] == nn) {
                    chip8->pc += 4;  // 跳过下一条指令
                } else {
                    chip8->pc += 2;  // 正常前进
                }
            }
            break;

        // ============ 4xxx: 条件跳过 ============
        case 0x4000: // 4XNN: 如果 VX != NN 则跳过 (SNE Vx, byte)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t nn = opcode & 0x00FF;
                printf("  执行: 4XNN (如果 V%X!=0x%02X 则跳过)\n", x, nn);
                
                if (chip8->V[x] != nn) {
                    chip8->pc += 4;
                } else {
                    chip8->pc += 2;
                }
            }
            break;

        // ============ 5xxx: 条件跳过 ============
        case 0x5000: // 5XY0: 如果 VX == VY 则跳过 (SE Vx, Vy)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                printf("  执行: 5XY0 (如果 V%X==V%X 则跳过)\n", x, y);
                
                if (chip8->V[x] == chip8->V[y]) {
                    chip8->pc += 4;
                } else {
                    chip8->pc += 2;
                }
            }
            break;

        // ============ 6xxx: 设置寄存器 ============
        case 0x6000: // 6XNN: 将常数NN存入寄存器VX (LD Vx, byte)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t nn = opcode & 0x00FF;
                printf("  执行: 6XNN (设置 V%X = 0x%02X)\n", x, nn);
                chip8->V[x] = nn;
                chip8->pc += 2;
            }
            break;

        // ============ 7xxx: 加法 ============
        case 0x7000: // 7XNN: VX = VX + NN (ADD Vx, byte)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t nn = opcode & 0x00FF;
                printf("  执行: 7XNN (V%X = V%X + 0x%02X)\n", x, x, nn);
                chip8->V[x] += nn;
                chip8->pc += 2;
            }
            break;

        // ============ 8xxx: 算术与逻辑 ============
        case 0x8000:
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                
                switch (opcode & 0x000F) {
                    case 0x0000: // 8XY0: VX = VY (LD Vx, Vy)
                        printf("  执行: 8XY0 (V%X = V%X)\n", x, y);
                        chip8->V[x] = chip8->V[y];
                        chip8->pc += 2;
                        break;
                        
                    case 0x0001: // 8XY1: VX = VX OR VY (OR Vx, Vy)
                        printf("  执行: 8XY1 (V%X = V%X | V%X)\n", x, x, y);
                        chip8->V[x] |= chip8->V[y];
                        chip8->pc += 2;
                        break;
                        
                    case 0x0002: // 8XY2: VX = VX AND VY (AND Vx, Vy)
                        printf("  执行: 8XY2 (V%X = V%X & V%X)\n", x, x, y);
                        chip8->V[x] &= chip8->V[y];
                        chip8->pc += 2;
                        break;
                        
                    case 0x0003: // 8XY3: VX = VX XOR VY (XOR Vx, Vy)
                        printf("  执行: 8XY3 (V%X = V%X ^ V%X)\n", x, x, y);
                        chip8->V[x] ^= chip8->V[y];
                        chip8->pc += 2;
                        break;
                        
                    case 0x0004: // 8XY4: VX = VX + VY (ADD Vx, Vy)
                        printf("  执行: 8XY4 (V%X = V%X + V%X)\n", x, x, y);
                        {
                            uint16_t sum = chip8->V[x] + chip8->V[y];
                            chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
                            chip8->V[x] = sum & 0xFF;
                        }
                        chip8->pc += 2;
                        break;
                        
                    case 0x0005: // 8XY5: VX = VX - VY (SUB Vx, Vy)
                        printf("  执行: 8XY5 (V%X = V%X - V%X)\n", x, x, y);
                        chip8->V[0xF] = (chip8->V[x] >= chip8->V[y]) ? 1 : 0;
                        chip8->V[x] -= chip8->V[y];
                        chip8->pc += 2;
                        break;
                        
                    case 0x0006: // 8XY6: VX = VX >> 1 (SHR Vx)
                        printf("  执行: 8XY6 (V%X = V%X >> 1)\n", x, x);
                        chip8->V[0xF] = chip8->V[x] & 0x01;
                        chip8->V[x] >>= 1;
                        chip8->pc += 2;
                        break;
                        
                    case 0x0007: // 8XY7: VX = VY - VX (SUBN Vx, Vy)
                        printf("  执行: 8XY7 (V%X = V%X - V%X)\n", x, y, x);
                        chip8->V[0xF] = (chip8->V[y] >= chip8->V[x]) ? 1 : 0;
                        chip8->V[x] = chip8->V[y] - chip8->V[x];
                        chip8->pc += 2;
                        break;
                        
                    case 0x000E: // 8XYE: VX = VX << 1 (SHL Vx)
                        printf("  执行: 8XYE (V%X = V%X << 1)\n", x, x);
                        chip8->V[0xF] = (chip8->V[x] & 0x80) >> 7;
                        chip8->V[x] <<= 1;
                        chip8->pc += 2;
                        break;
                        
                    default:
                        printf("  未实现的8指令: 0x%04X\n", opcode);
                        chip8->pc += 2;
                        break;
                }
            }
            break;

        // ============ 9xxx: 条件跳过 ============
        case 0x9000: // 9XY0: 如果 VX != VY 则跳过 (SNE Vx, Vy)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                printf("  执行: 9XY0 (如果 V%X!=V%X 则跳过)\n", x, y);
                
                if (chip8->V[x] != chip8->V[y]) {
                    chip8->pc += 4;
                } else {
                    chip8->pc += 2;
                }
            }
            break;

        // ============ Axxx: 设置索引寄存器 ============
        case 0xA000: // ANNN: 设置I寄存器 (LD I, addr)
            {
                uint16_t address = opcode & 0x0FFF;
                printf("  执行: ANNN (设置 I = 0x%03X)\n", address);
                chip8->I = address;
                chip8->pc += 2;
            }
            break;

        // ============ Bxxx: 跳转 ============
        case 0xB000: // BNNN: 跳转到地址 NNN + V0 (JP V0, addr)
            {
                uint16_t address = opcode & 0x0FFF;
                printf("  执行: BNNN (跳转到 0x%03X + V0)\n", address);
                chip8->pc = address + chip8->V[0];
            }
            break;

        // ============ Cxxx: 随机数 ============
        case 0xC000: // CXNN: VX = 随机数 & NN (RND Vx, byte)
            {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t nn = opcode & 0x00FF;
                printf("  执行: CXNN (V%X = 随机数 & 0x%02X)\n", x, nn);
                
                // 简单随机数生成（实际应使用更好的算法）
                static unsigned int seed = 12345;
                seed = (seed * 1103515245 + 12345) % 0x7FFFFFFF;
                chip8->V[x] = (seed & 0xFF) & nn;
                chip8->pc += 2;
            }
            break;

        // ============ Dxxx: 显示绘图 ============
        case 0xD000: // DXYN: 绘制精灵 (DRW Vx, Vy, n)
            {
                uint8_t x = chip8->V[(opcode & 0x0F00) >> 8];
                uint8_t y = chip8->V[(opcode & 0x00F0) >> 4];
                uint8_t height = opcode & 0x000F;
                uint8_t pixel;

                printf("  执行: DXYN (在 X=%u, Y=%u 绘制 %u 行高精灵)\n", x, y, height);

                chip8->V[0xF] = 0;

                for (int yline = 0; yline < height; yline++) {
                    pixel = chip8->memory[chip8->I + yline];
                    
                    for (int xline = 0; xline < 8; xline++) {
                        if ((pixel & (0x80 >> xline)) != 0) {
                            int display_x = (x + xline) % DISPLAY_WIDTH;
                            int display_y = (y + yline) % DISPLAY_HEIGHT;
                            int pixel_index = display_y * DISPLAY_WIDTH + display_x;

                            if (chip8->display[pixel_index] == 1) {
                                chip8->V[0xF] = 1;
                            }

                            chip8->display[pixel_index] ^= 1;
                        }
                    }
                }

                chip8->draw_flag = 1;
                chip8->pc += 2;
            }
            break;

        // ============ Exxx: 键盘输入 ============
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // EX9E: 如果按键 VX 被按下，则跳过下一条指令 (SKP Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        uint8_t key_to_check = chip8->V[x];
                        printf("  执行: EX9E (如果键0x%X按下则跳过)\n", key_to_check);
                        
                        if (key_to_check < 16 && chip8->key[key_to_check]) {
                            chip8->pc += 4;  // 跳过下一条指令
                        } else {
                            chip8->pc += 2;  // 正常前进
                        }
                    }
                    break;
                    
                case 0x00A1: // EXA1: 如果按键 VX 没被按下，则跳过下一条指令 (SKNP Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        uint8_t key_to_check = chip8->V[x];
                        printf("  执行: EXA1 (如果键0x%X没按下则跳过)\n", key_to_check);
                        
                        if (key_to_check < 16 && !chip8->key[key_to_check]) {
                            chip8->pc += 4;
                        } else {
                            chip8->pc += 2;
                        }
                    }
                    break;
                    
                default:
                    printf("  未实现的E指令: 0x%04X\n", opcode);
                    chip8->pc += 2;
                    break;
            }
            break;

        // ============ Fxxx: 杂项指令 ============
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // FX07: VX = 延迟定时器 (LD Vx, DT)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX07 (V%X = 延迟定时器=%u)\n", x, chip8->delay_timer);
                        chip8->V[x] = chip8->delay_timer;
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x000A: // FX0A: 等待按键，然后存入 VX (LD Vx, K)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX0A (等待按键存入 V%X)\n", x);
                        
                        // 检查是否有按键被按下
                        uint8_t key_pressed = 0xFF;
                        for (int i = 0; i < 16; i++) {
                            if (chip8->key[i]) {
                                key_pressed = i;
                                break;
                            }
                        }
                        
                        if (key_pressed != 0xFF) {
                            // 有按键被按下
                            chip8->V[x] = key_pressed;
                            chip8->pc += 2;
                            printf("    按键 0x%X 按下，存入 V%X\n", key_pressed, x);
                        } else {
                            // 没有按键被按下，PC不前进，下个周期再检查
                            printf("    等待按键中...\n");
                        }
                    }
                    break;
                    
                case 0x0015: // FX15: 设置延迟定时器 (LD DT, Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX15 (延迟定时器 = V%X=%u)\n", x, chip8->V[x]);
                        chip8->delay_timer = chip8->V[x];
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x0018: // FX18: 设置声音定时器 (LD ST, Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX18 (声音定时器 = V%X=%u)\n", x, chip8->V[x]);
                        chip8->sound_timer = chip8->V[x];
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x001E: // FX1E: I = I + VX (ADD I, Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX1E (I = I + V%X)\n", x);
                        chip8->I += chip8->V[x];
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x0029: // FX29: I = 字体字符地址 (LD F, Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        uint8_t digit = chip8->V[x] & 0x0F; // 只取低4位
                        printf("  执行: FX29 (I = 字体'%X'地址=0x%03X)\n", 
                               digit, digit * 5);
                        chip8->I = digit * 5; // 每个字符5字节
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x0033: // FX33: 二进制十进制转换 (LD B, Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        uint8_t value = chip8->V[x];
                        printf("  执行: FX33 (将 V%X=%u 转为BCD)\n", x, value);
                        
                        // 百位
                        chip8->memory[chip8->I] = value / 100;
                        // 十位
                        chip8->memory[chip8->I + 1] = (value / 10) % 10;
                        // 个位
                        chip8->memory[chip8->I + 2] = value % 10;
                        
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x0055: // FX55: 保存寄存器到内存 (LD [I], Vx)
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX55 (保存 V0-V%X 到内存)\n", x);
                        
                        for (int i = 0; i <= x; i++) {
                            chip8->memory[chip8->I + i] = chip8->V[i];
                        }
                        
                        chip8->pc += 2;
                    }
                    break;
                    
                case 0x0065: // FX65: 从内存加载寄存器 (LD Vx, [I])
                    {
                        uint8_t x = (opcode & 0x0F00) >> 8;
                        printf("  执行: FX65 (从内存加载到 V0-V%X)\n", x);
                        
                        for (int i = 0; i <= x; i++) {
                            chip8->V[i] = chip8->memory[chip8->I + i];
                        }
                        
                        chip8->pc += 2;
                    }
                    break;
                    
                default:
                    printf("  未实现的F指令: 0x%04X\n", opcode);
                    chip8->pc += 2;
                    break;
            }
            break;

        default:
            printf("  未知指令类型: 0x%04X\n", opcode);
            chip8->pc += 2;
            break;
    }
}

// 更新定时器（应在约60Hz的频率下调用）
void chip8_update_timers(Chip8* chip8) {
    if (chip8->delay_timer > 0) {
        chip8->delay_timer--;
    }
    
    if (chip8->sound_timer > 0) {
        if (chip8->sound_timer == 1) {
            // 这里可以触发声音播放
            // printf("BEEP!\n");
        }
        chip8->sound_timer--;
    }
}

// 初始化SDL2图形系统
int chip8_graphics_init(Chip8* chip8) {
    if (!chip8) return 0;
    
    // 1. 初始化SDL2视频子系统
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL2初始化失败: %s\n", SDL_GetError());
        return 0;
    }
    
    // 2. 创建窗口
    chip8->window = SDL_CreateWindow(
        "CHIP-8",               // 窗口标题
        SDL_WINDOWPOS_CENTERED,        // 初始X位置
        SDL_WINDOWPOS_CENTERED,        // 初始Y位置
        WINDOW_WIDTH,                  // 窗口宽度
        WINDOW_HEIGHT,                 // 窗口高度
        SDL_WINDOW_SHOWN               // 显示窗口标志
    );
    
    if (!chip8->window) {
        fprintf(stderr, "创建窗口失败: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }
    
    // 3. 创建渲染器（用于绘制）
    chip8->renderer = SDL_CreateRenderer(
        chip8->window,
        -1,                            // 使用第一个可用的渲染驱动
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!chip8->renderer) {
        fprintf(stderr, "创建渲染器失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(chip8->window);
        SDL_Quit();
        return 0;
    }
    
    printf("图形系统初始化成功 (窗口: %dx%d)\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    return 1;
}

// 更新图形显示：将display数组中的像素绘制到窗口
void chip8_graphics_update(Chip8* chip8) {
    if (!chip8 || !chip8->renderer) return;
    
    // 1. 设置绘制颜色为黑色（背景）
    SDL_SetRenderDrawColor(chip8->renderer, 0, 0, 0, 255); // 黑色背景
    SDL_RenderClear(chip8->renderer); // 清屏
    
    // 2. 设置绘制颜色为白色（前景/像素）
    SDL_SetRenderDrawColor(chip8->renderer, 255, 255, 255, 255); // 白色像素
    
    // 3. 遍历CHIP-8的显示缓冲区(64x32)，将“点亮”的像素绘制到窗口
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            // 如果该像素为“开”(值为1)
            if (chip8->display[y * DISPLAY_WIDTH + x]) {
                // 计算放大后的矩形位置和大小
                SDL_Rect pixel_rect = {
                    x * WINDOW_SCALE,      // 放大后的X坐标
                    y * WINDOW_SCALE,      // 放大后的Y坐标
                    WINDOW_SCALE,          // 像素宽度
                    WINDOW_SCALE           // 像素高度
                };
                // 绘制一个实心矩形代表一个放大的像素
                SDL_RenderFillRect(chip8->renderer, &pixel_rect);
            }
        }
    }
    
    // 4. 将渲染结果提交到屏幕
    SDL_RenderPresent(chip8->renderer);
}

// 清理图形资源
void chip8_graphics_cleanup(Chip8* chip8) {
    if (!chip8) return;
    
    if (chip8->renderer) {
        SDL_DestroyRenderer(chip8->renderer);
        chip8->renderer = NULL;
    }
    if (chip8->window) {
        SDL_DestroyWindow(chip8->window);
        chip8->window = NULL;
    }
    
    SDL_Quit();
    printf("图形资源已清理\n");
}