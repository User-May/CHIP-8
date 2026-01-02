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
    printf("=== CHIP-8 模拟器启动 ===\n");

    // 创建并初始化CPU
    Chip8 chip8;
    chip8_init(&chip8);
    
    // 加载ROM
    const char* rom_filename = (argc > 1) ? argv[1] : "test_real.ch8";
    if (!chip8_load_rom(&chip8, rom_filename)) {
        fprintf(stderr, "错误：无法加载ROM '%s'，程序退出。\n", rom_filename);
        return 1;
    }
    printf("ROM加载成功。\n");
    
    // ===== 【关键修改】增强错误检查的图形初始化 =====
    printf("正在初始化SDL2图形系统...\n");
    if (!chip8_graphics_init(&chip8)) {
        // chip8_graphics_init 内部会打印错误，这里直接退出
        fprintf(stderr, "图形系统初始化失败，程序退出。\n");
        return 1;
    }
    printf("SDL2图形系统初始化成功！\n");
    printf("提示：按ESC键或关闭窗口退出。\n");
    
    // ==================== 主循环 ====================
    int is_running = 1;
    SDL_Event event;
    
    while (is_running) {
        // 1. 处理事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = 0;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                is_running = 0;
            }
        }
        
        // 2. 执行一个CPU周期
        chip8_cycle(&chip8);
        
        // 3. 如果需要，更新显示
        if (chip8.draw_flag) {
            chip8_graphics_update(&chip8);
            chip8.draw_flag = 0;
        }
        
        // 可选：降低CPU占用，控制模拟速度
        SDL_Delay(1); // 延时1毫秒
    }
    
    // ==================== 清理退出 ====================
    printf("正在清理资源...\n");
    chip8_graphics_cleanup(&chip8);
    printf("模拟器正常退出。\n");
    return 0;
}