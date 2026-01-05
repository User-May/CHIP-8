// main.c - CHIP-8模拟器主程序
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_timer.h>
#include "chip8.h"

// 动画速度控制
static int animation_speed = ANIMATION_DEFAULT_SPEED; // 动画速度 (1-20)
static int frame_counter = 0;                         // 帧计数器

// 改变动画速度
void change_animation_speed(int delta) {
    int old_speed = animation_speed;
    animation_speed += delta;
    
    // 限制速度范围
    if (animation_speed < ANIMATION_MIN_SPEED) {
        animation_speed = ANIMATION_MIN_SPEED;
    }
    if (animation_speed > ANIMATION_MAX_SPEED) {
        animation_speed = ANIMATION_MAX_SPEED;
    }
    
    // 如果速度改变了，打印消息
    if (old_speed != animation_speed) {
        printf("动画速度改变: %d -> %d (O=加速动画, P=减速动画)\n", old_speed, animation_speed);
        
        // 计算当前图形刷新间隔（毫秒）
        // 公式：刷新间隔 = 基础间隔 * (最大速度 - 当前速度 + 1) / 最大速度
        int base_refresh_interval = 33; // 基础刷新间隔约30fps (1000ms/30)
        int current_interval = base_refresh_interval * (ANIMATION_MAX_SPEED - animation_speed + 1) / ANIMATION_MAX_SPEED;
        if (current_interval < 1) current_interval = 1;
        
        printf("当前图形刷新间隔: %dms (约%d FPS)\n", 
               current_interval, 1000 / current_interval);
    }
}

// 键盘映射：将PC键盘按键映射到CHIP-8的16键键盘
void handle_key_event(Chip8* chip8, SDL_KeyboardEvent* key) {
    uint8_t chip8_key = 0xFF;
    
    // 忽略按键重复事件
    if (key->repeat) {
        return;
    }
    
    // 打印按下的键
    if (key->type == SDL_KEYDOWN) {
        printf("按键: %s\n", SDL_GetKeyName(key->keysym.sym));
    }
    
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
        
        // B键测试声音 - 改为短蜂鸣
        case SDLK_b:  
            if (key->type == SDL_KEYDOWN) {
                printf("=== 触发短蜂鸣测试 ===\n");
                chip8->sound_timer = 12;  // 改为12（约0.2秒）
                printf("声音定时器设置为: %u (约%.1f秒)\n", 
                       chip8->sound_timer, (float)chip8->sound_timer / 60.0f);
            }
            break;
            
        // O键加速动画
        case SDLK_o:
            if (key->type == SDL_KEYDOWN) {
                printf("=== 加速动画 ===\n");
                change_animation_speed(1);  // 增加动画速度
            }
            break;
            
        // P键减速动画
        case SDLK_p:
            if (key->type == SDL_KEYDOWN) {
                printf("=== 减速动画 ===\n");
                change_animation_speed(-1);  // 降低动画速度
            }
            break;
            
        default: break;
    }
    
    if (chip8_key != 0xFF) {
        chip8->key[chip8_key] = (key->type == SDL_KEYDOWN) ? 1 : 0;
    }
}

int main(int argc, char* argv[]) {
    printf("CHIP-8 模拟器\n");
    printf("===============\n");
    
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
    
    // 初始化音频系统
    printf("正在初始化音频系统...\n");
    if (!chip8_audio_init(&chip8)) {
        fprintf(stderr, "警告: 音频系统初始化失败，继续无声音运行\n");
    } else {
        printf("音频系统初始化成功\n");
    }
    
    printf("图形系统初始化成功\n");
    printf("控制说明:\n");
    printf("  W/A/S/D - 上/左/下/右移动\n");
    printf("  R       - 重置位置\n");
    printf("  B       - 测试蜂鸣声（短蜂鸣）\n");
    printf("  O       - 加速动画 (增加图形刷新速度)\n");
    printf("  P       - 减速动画 (降低图形刷新速度)\n");
    printf("  ESC     - 退出程序\n");
    printf("\n");
    printf("当前动画速度: %d/%d (1=最慢, 20=最快)\n", 
           animation_speed, ANIMATION_MAX_SPEED);
    printf("CPU模拟速度保持恒定 (标准CHIP-8速度)\n");
    printf("按 O 和 P 键可实时调整动画速度\n");
    printf("开始运行模拟器...\n");
    
    // 主循环
    int is_running = 1;
    SDL_Event event;
    
    // 用于控制CPU执行速度的计时器
    Uint32 cpu_last_time = SDL_GetTicks();
    Uint32 cpu_accumulator = 0;
    const Uint32 cpu_target_interval = 2; // CPU每2ms执行一次 (约500Hz)
    
    // 用于控制图形刷新速度的计时器
    Uint32 graphics_last_time = SDL_GetTicks();
    
    // 计算当前图形刷新间隔（毫秒）
    // 公式：刷新间隔 = 基础间隔 * (最大速度 - 当前速度 + 1) / 最大速度
    int base_refresh_interval = 33; // 基础刷新间隔约30fps (1000ms/30)
    int graphics_refresh_interval = base_refresh_interval * 
                                   (ANIMATION_MAX_SPEED - animation_speed + 1) / 
                                   ANIMATION_MAX_SPEED;
    if (graphics_refresh_interval < 1) graphics_refresh_interval = 1;
    
    while (is_running) {
        Uint32 current_time = SDL_GetTicks();
        
        // 1. 处理事件（每次循环都处理，确保响应及时）
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
        
        // 2. CPU执行（固定频率，不受动画速度影响）
        cpu_accumulator += (current_time - cpu_last_time);
        cpu_last_time = current_time;
        
        // 每2ms执行一次CPU周期
        while (cpu_accumulator >= cpu_target_interval) {
            // 执行CPU周期（固定速度）
            chip8_cycle(&chip8);
            
            // 更新定时器（60Hz）- 每8个CPU周期更新一次
            static int timer_counter = 0;
            if (++timer_counter >= 8) {
                chip8_update_timers(&chip8);
                timer_counter = 0;
            }
            
            cpu_accumulator -= cpu_target_interval;
        }
        
        // 3. 图形刷新（受动画速度控制）
        Uint32 graphics_elapsed = current_time - graphics_last_time;
        if (graphics_elapsed >= graphics_refresh_interval) {
            // 重新计算图形刷新间隔（因为动画速度可能已改变）
            graphics_refresh_interval = base_refresh_interval * 
                                       (ANIMATION_MAX_SPEED - animation_speed + 1) / 
                                       ANIMATION_MAX_SPEED;
            if (graphics_refresh_interval < 1) graphics_refresh_interval = 1;
            
            // 更新显示
            if (chip8.draw_flag) {
                chip8_graphics_update(&chip8);
                chip8.draw_flag = 0;
                frame_counter++;
                
                // 每60帧显示一次状态
                if (frame_counter % 60 == 0) {
                    printf("运行状态: 帧数=%d, PC=0x%03X, 声音定时器=%u, 动画速度=%d/%d\n", 
                           frame_counter, chip8.pc, chip8.sound_timer, 
                           animation_speed, ANIMATION_MAX_SPEED);
                }
            }
            
            graphics_last_time = current_time;
        }
        
        // 4. 短暂延迟以避免过度占用CPU
        SDL_Delay(1);
    }
    
    // 清理资源
    printf("正在清理资源...\n");
    chip8_graphics_cleanup(&chip8);
    printf("模拟器已关闭\n");
    
    return 0;
}