#include <stdio.h>
#include <string.h>
#include "chip8.h"

// 键盘映射：将PC键盘按键映射到CHIP-8的16键键盘
// CHIP-8的原始布局:   PC映射（常见）:
// 1 2 3 C           1 2 3 4
// 4 5 6 D    ->     Q W E R
// 7 8 9 E           A S D F
// A 0 B F           Z X C V
void handle_key_event(Chip8* chip8, SDL_KeyboardEvent* key) {
    uint8_t chip8_key = 0xFF; // 0xFF 表示未映射的键
    
    // 映射 SDL 键码到 CHIP-8 键 (0x0-0xF)
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
        
        default: break; // 其他键不处理
    }
    
    // 如果键被映射，则更新 CHIP-8 的键盘状态数组
    if (chip8_key != 0xFF) {
        if (key->type == SDL_KEYDOWN) {
            chip8->key[chip8_key] = 1; // 按键按下
        } else if (key->type == SDL_KEYUP) {
            chip8->key[chip8_key] = 0; // 按键释放
        }
    }
}

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
    printf("=== CHIP-8 模拟器 (带键盘输入) ===\n");
    printf("构建日期: %s %s\n", __DATE__, __TIME__);
    
    // 创建并初始化CPU
    Chip8 chip8;
    chip8_init(&chip8);
    
    // 加载ROM - 使用移动演示或默认ROM
    const char* rom_filename = "move_demo.ch8";  // 默认使用移动演示
    if (argc > 1) {
        rom_filename = argv[1];
    }
    
    if (!chip8_load_rom(&chip8, rom_filename)) {
        fprintf(stderr, "错误: 无法加载ROM '%s'\n", rom_filename);
        fprintf(stderr, "请确保文件存在且大小合适。\n");
        return 1;
    }
    
    printf("ROM '%s' 加载成功。\n", rom_filename);
    
    // 初始化图形系统
    if (!chip8_graphics_init(&chip8)) {
        fprintf(stderr, "图形系统初始化失败，程序退出。\n");
        return 1;
    }
    printf("SDL2图形系统初始化成功！\n");
    printf("提示: 按ESC键退出程序\n");
    
    if (strstr(rom_filename, "move_demo") != NULL) {
        printf("控制说明:\n");
        printf("  W/A/S/D - 上/左/下/右移动像素\n");
        printf("  R - 重置到中心位置\n");
        printf("  ESC - 退出程序\n");
    }
    
    // ==================== 主循环 ====================
    printf("\n进入主循环...\n");
    
    int is_running = 1;
    SDL_Event event;
    int frame_count = 0;
    
    while (is_running) {
        // 1. 处理所有SDL事件
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    printf("收到退出事件，正在退出...\n");
                    is_running = 0;
                    break;
                    
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    handle_key_event(&chip8, &event.key);
                    
                    // ESC键退出（可选）
                    if (event.type == SDL_KEYDOWN && 
                        event.key.keysym.sym == SDLK_ESCAPE) {
                        printf("ESC键按下，正在退出...\n");
                        is_running = 0;
                    }
                    break;
                    
                default:
                    // 其他事件忽略
                    break;
            }
        }
        
        // 2. 执行一个CHIP-8 CPU周期
        chip8_cycle(&chip8);
        
        // 3. 更新定时器（60Hz）
        // 这里简化处理，每帧都更新
        static int timer_counter = 0;
        if (++timer_counter >= 8) { // 假设模拟器以~480Hz运行，60Hz是480/8
            chip8_update_timers(&chip8);
            timer_counter = 0;
        }
        
        // 4. 如果需要，更新图形显示
        if (chip8.draw_flag) {
            chip8_graphics_update(&chip8);
            chip8.draw_flag = 0;
            frame_count++;
            
            // 每60帧显示一次状态（可选调试）
            if (frame_count % 60 == 0) {
                printf("运行中... 帧数: %d, PC: 0x%03X, 按键状态: ", 
                       frame_count, chip8.pc);
                for (int i = 0; i < 4; i++) {
                    printf("%d", chip8.key[i]);
                }
                printf(" ");
                for (int i = 4; i < 8; i++) {
                    printf("%d", chip8.key[i]);
                }
                printf(" ");
                for (int i = 8; i < 12; i++) {
                    printf("%d", chip8.key[i]);
                }
                printf(" ");
                for (int i = 12; i < 16; i++) {
                    printf("%d", chip8.key[i]);
                }
                printf("\n");
            }
        }
        
        // 5. 短暂延迟以控制模拟速度
        SDL_Delay(2); // ~500Hz
    }
    
    // ==================== 清理退出 ====================
    printf("\n正在清理资源...\n");
    chip8_graphics_cleanup(&chip8);
    printf("模拟器正常退出。\n");
    
    return 0;
}