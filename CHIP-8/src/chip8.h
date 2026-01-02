#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <SDL2/SDL.h>

// 内存大小 - 4KB (4096 bytes)
#define MEMORY_SIZE 4096
#define PROGRAM_START 0x200  // CHIP-8程序起始地址
#define DISPLAY_WIDTH 64     // 显示宽度
#define DISPLAY_HEIGHT 32    // 显示高度

// 图形显示常量
#define WINDOW_SCALE 10 // 窗口缩放倍数 (CHIP-8原始分辨率64x32，放大10倍后为640x320)
#define WINDOW_WIDTH (DISPLAY_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (DISPLAY_HEIGHT * WINDOW_SCALE)

// CPU结构体
typedef struct {
    // 内存
    uint8_t memory[MEMORY_SIZE];
    
    // 寄存器
    uint8_t V[16];            // 16个8位通用寄存器 (V0-VF)
    uint16_t I;               // 16位地址寄存器
    uint16_t pc;              // 程序计数器
    
    // 堆栈
    uint16_t stack[16];       // 16层堆栈
    uint8_t sp;               // 堆栈指针
    
    // 定时器
    uint8_t delay_timer;      // 延迟定时器
    uint8_t sound_timer;      // 声音定时器
    
    // 显示
    uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT];  // 像素缓冲区 (0=关, 1=开)
    
    // 键盘输入 (16键: 0-9, A-F)
    uint8_t key[16];
    
    // 状态标志
    uint8_t draw_flag;        // 需要重绘显示
    uint8_t key_wait;         // 等待按键按下
    uint8_t key_reg;          // 等待按键的寄存器

    SDL_Window* window;      // 窗口
    SDL_Renderer* renderer;  // 渲染器
} Chip8;

// 函数声明
void chip8_init(Chip8* chip8);
int chip8_load_rom(Chip8* chip8, const char* filename);
void chip8_cycle(Chip8* chip8);
void chip8_update_timers(Chip8* chip8);
int chip8_graphics_init(Chip8* chip8);    // 初始化图形
void chip8_graphics_update(Chip8* chip8); // 更新图形显示
void chip8_graphics_cleanup(Chip8* chip8);// 清理图形资源

#endif // CHIP8_H