// main.c - CHIP-8模拟器主程序
#include <stdio.h>
#include <string.h>
#include "chip8.h"

// 键盘映射：将PC键盘按键映射到CHIP-8的16键键盘
void handle_key_event(Chip8* chip8, SDL_KeyboardEvent* key) {
    uint8_t chip8_key = 0xFF;
    // 打印按下的键
    printf("SDL按键: %s (%d) ",SDL_GetKeyName(key->keysym.sym),key->keysym.sym);
    switch (key->keysym.sym) {
        case SDLK_1: chip8_key = 0x1; break;
        case SDLK_2: chip8_key = 0x2; break;
        case SDLK_3: chip8_key = 0x3; break;
        case SDLK_4: chip8_key = 0xC; break;
        
        case SDLK_q: chip8_key = 0x4; break;
        case SDLK_w: chip8_key = 0x5; break;
        case SDLK_e: chip8_key = 0x6; break;
        case SDLK_r: chip8_key = 0xD; break;
        
        case SDLK_a: chip8_key = 0x7; break;
        case SDLK_s: chip8_key = 0x8; break;
        case SDLK_d: chip8_key = 0x9; break;
        case SDLK_f: chip8_key = 0xE; break;
        
        case SDLK_z: chip8_key = 0xA; break;
        case SDLK_x: chip8_key = 0x0; break;
        case SDLK_c: chip8_key = 0xB; break;
        case SDLK_v: chip8_key = 0xF; break;
        
        default: break;
    }
    
    if (chip8_key != 0xFF) {
        chip8->key[chip8_key] = (key->type == SDL_KEYDOWN) ? 1 : 0;
    }
}

int main(int argc, char* argv[]) {
    printf("CHIP-8 模拟器\n");
    
    // 初始化CHIP-8系统
    Chip8 chip8;
    chip8_init(&chip8);
    
    // 确定ROM文件名
    const char* rom_filename = (argc > 1) ? argv[1] : "move_demo.ch8";
    printf("正在加载ROM文件: %s\n", rom_filename);
    
    // 加载ROM
    if (!chip8_load_rom(&chip8, rom_filename)) {
        fprintf(stderr, "错误: 无法加载ROM文件 '%s'\n", rom_filename);
        fprintf(stderr, "请确保文件存在且大小合适\n");
        return 1;
    }
    
    printf("ROM加载成功\n");
    
    // 初始化图形系统
    printf("正在初始化图形系统...\n");
    if (!chip8_graphics_init(&chip8)) {
        fprintf(stderr, "错误: 图形系统初始化失败\n");
        return 1;
    }
    
    printf("图形系统初始化成功\n");
    printf("控制说明:\n");
    printf("  W/A/S/D - 上/左/下/右移动\n");
    printf("  R       - 重置位置\n");
    printf("  ESC     - 退出程序\n");
    printf("开始运行模拟器...\n");
    
    // 主循环
    int is_running = 1;
    SDL_Event event;
    int frame_count = 0;
    
    while (is_running) {
        // 处理事件
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    printf("收到退出事件\n");
                    is_running = 0;
                    break;
                    
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    handle_key_event(&chip8, &event.key);
                    
                    // ESC键退出
                    if (event.type == SDL_KEYDOWN && 
                        event.key.keysym.sym == SDLK_ESCAPE) {
                        printf("ESC键按下，退出程序\n");
                        is_running = 0;
                    }
                    break;
            }
        }
        
        // 执行CPU周期
        chip8_cycle(&chip8);
        
        // 更新定时器（60Hz）
        static int timer_counter = 0;
        if (++timer_counter >= 8) {
            chip8_update_timers(&chip8);
            timer_counter = 0;
        }
        
        // 更新显示
        if (chip8.draw_flag) {
            chip8_graphics_update(&chip8);
            chip8.draw_flag = 0;
            frame_count++;
            
            // 每60帧显示一次状态
            if (frame_count % 60 == 0) {
                printf("运行状态: 帧数=%d, PC=0x%03X\n", frame_count, chip8.pc);
            }
        }
        
        // 控制模拟速度
        SDL_Delay(2);
    }
    
    // 清理资源
    printf("正在清理资源...\n");
    chip8_graphics_cleanup(&chip8);
    printf("模拟器已关闭\n");
    
    return 0;
}