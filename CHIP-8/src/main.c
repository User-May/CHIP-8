// main.c - CHIP-8模拟器主程序
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <SDL2/SDL_timer.h>
#include "chip8.h"

// 游戏速度控制
static int game_speed = CPU_DEFAULT_SPEED;  // 游戏速度 (指令/秒)
static int frame_counter = 0;              // 帧计数器
static Uint32 last_fps_time = 0;           // 上次计算FPS的时间
static int frame_count_since_last = 0;     // 上次计算FPS以来的帧数
static float current_fps = 0.0f;           // 当前FPS
static Uint32 last_cycle_time = 0;         // 上次执行CPU周期的时间
static float cycle_accumulator = 0.0f;     // 累积的时间（用于CPU周期）

// 函数声明
void change_game_speed(int delta);
void handle_key_event(Chip8* chip8, SDL_KeyboardEvent* key);
void update_fps_display(void);
int load_and_run_rom(Chip8* chip8, const char* rom_path);

// 检查文件扩展名是否为.ch8（不区分大小写）
int is_ch8_file(const char* filename) {
    if (!filename) return 0;
    
    // 查找文件扩展名
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return 0;
    }
    
    // 检查扩展名是否为.ch8（不区分大小写）
    return (strcasecmp(dot, ".ch8") == 0);
}

// 加载并运行ROM文件
int load_and_run_rom(Chip8* chip8, const char* rom_path) {
    if (!chip8 || !rom_path) {
        fprintf(stderr, "错误: 加载ROM参数为空\n");
        return 0;
    }
    
    // 检查文件扩展名
    if (!is_ch8_file(rom_path)) {
        fprintf(stderr, "错误: 文件 '%s' 不是.ch8格式的ROM文件\n", rom_path);
        return 0;
    }
    
    printf("正在加载ROM文件: %s\n", rom_path);
    
    // 重置CHIP-8系统
    chip8_init(chip8);
    
    // 加载ROM
    if (!chip8_load_rom(chip8, rom_path)) {
        fprintf(stderr, "错误: 无法加载ROM文件 '%s'\n", rom_path);
        fprintf(stderr, "请确保文件存在且大小合适\n");
        return 0;
    }
    
    printf("ROM加载成功: %s\n", rom_path);
    printf("文件路径: %s\n", rom_path);
    printf("按 ESC 键退出, 按 O/P 键调整游戏速度\n");
    
    return 1;
}

// 改变游戏速度
void change_game_speed(int delta) {
    int old_speed = game_speed;
    
    // 逐步调整速度：100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000
    if (delta > 0) {
        // 加速
        if (game_speed < 1000) {
            game_speed += 100;
        } else {
            game_speed = 2000;  // 从1000直接到2000
        }
    } else {
        // 减速
        if (game_speed > 100) {
            if (game_speed == 2000) {
                game_speed = 1000;
            } else {
                game_speed -= 100;
            }
        }
    }
    
    // 限制速度范围
    if (game_speed < CPU_MIN_SPEED) {
        game_speed = CPU_MIN_SPEED;
    }
    if (game_speed > CPU_MAX_SPEED) {
        game_speed = CPU_MAX_SPEED;
    }
    
    // 如果速度改变了，打印消息
    if (old_speed != game_speed) {
        printf("游戏速度改变: %d -> %d 指令/秒 (O=减速, P=加速)\n", old_speed, game_speed);
        printf("速度级别: ");
        if (game_speed == 100) printf("极慢");
        else if (game_speed == 200) printf("很慢");
        else if (game_speed == 300) printf("慢");
        else if (game_speed == 400) printf("较慢");
        else if (game_speed == 500) printf("正常");
        else if (game_speed == 600) printf("较快");
        else if (game_speed == 700) printf("快");
        else if (game_speed == 800) printf("很快");
        else if (game_speed == 900) printf("极快");
        else if (game_speed == 1000) printf("超快");
        else if (game_speed == 2000) printf("极限");
        printf("\n");
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
        
        // B键-短蜂鸣声
        case SDLK_b:  
            if (key->type == SDL_KEYDOWN) {
                printf("=== 触发短蜂鸣测试 ===\n");
                chip8->sound_timer = 12;  // 改为12（约0.2秒）
                printf("声音定时器设置为: %u (约%.1f秒)\n", 
                       chip8->sound_timer, (float)chip8->sound_timer / 60.0f);
            }
            break;
            
        // O键减速
        case SDLK_o:
            if (key->type == SDL_KEYDOWN) {
                printf("=== 减速 ===\n");
                change_game_speed(-1);  // 降低游戏速度
            }
            break;
            
        // P键加速
        case SDLK_p:
            if (key->type == SDL_KEYDOWN) {
                printf("=== 加速 ===\n");
                change_game_speed(1);  // 增加游戏速度
            }
            break;
            
        default: break;
    }
    
    if (chip8_key != 0xFF) {
        chip8->key[chip8_key] = (key->type == SDL_KEYDOWN) ? 1 : 0;
    }
}

// 更新FPS显示
void update_fps_display(void) {
    frame_count_since_last++;
    
    Uint32 current_time = SDL_GetTicks();
    Uint32 elapsed = current_time - last_fps_time;
    
    // 每500毫秒更新一次FPS显示
    if (elapsed >= 500) {
        current_fps = (float)frame_count_since_last / (elapsed / 1000.0f);
        last_fps_time = current_time;
        frame_count_since_last = 0;
    }
}

int main(int argc, char* argv[]) {
    // 初始化CHIP-8
    Chip8 chip8;
    chip8_init(&chip8);
    
    // 确定初始ROM文件名（如果通过命令行参数指定）
    const char* initial_rom_filename = NULL;
    if (argc > 1) {
        initial_rom_filename = argv[1];
        printf("检测到命令行参数，尝试加载ROM: %s\n", initial_rom_filename);
    } else {
        printf("未指定ROM文件，请将.ch8格式的ROM文件拖放到窗口中\n");
        printf("或通过命令行参数指定ROM文件路径\n");
    }
    
    // 初始化图形系统
    printf("正在初始化图形系统...\n");
    if (!chip8_graphics_init(&chip8)) {
        fprintf(stderr, "错误: 图形系统初始化失败\n");
        return 1;
    }
    
    // 启用SDL拖放功能
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    
    // 初始化音频系统
    printf("正在初始化音频系统...\n");
    if (!chip8_audio_init(&chip8)) {
        fprintf(stderr, "警告: 音频系统初始化失败，继续无声音运行\n");
    } else {
        printf("音频系统初始化成功\n");
    }
    
    printf("图形系统初始化成功\n");
    printf("初始游戏速度: %d 指令/秒\n", game_speed);
    printf("游戏速度范围: %d-%d 指令/秒 (O=减速, P=加速)\n", CPU_MIN_SPEED, CPU_MAX_SPEED);
    printf("速度级别: 100=极慢, 200=很慢, 300=慢, 400=较慢, 500=正常, 600=较快, 700=快, 800=很快, 900=极快, 1000=超快, 2000=极限\n");
    
    // 如果提供了命令行参数，尝试加载ROM
    int rom_loaded = 0;
    if (initial_rom_filename) {
        if (load_and_run_rom(&chip8, initial_rom_filename)) {
            rom_loaded = 1;
        }
    }
    
    if (!rom_loaded) {
        printf("等待ROM文件...\n");
        printf("请将.ch8格式的ROM文件拖放到窗口中\n");
    }
    
    printf("开始运行模拟器...\n");
    
    // 主循环
    int is_running = 1;
    SDL_Event event;
    
    // 初始化计时器
    last_cycle_time = SDL_GetTicks();
    last_fps_time = SDL_GetTicks();
    
    // 定时器更新计数器（60Hz）
    static int timer_counter = 0;
    static Uint32 last_timer_update = 0;
    
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
                    
                case SDL_DROPFILE:
                    // 拖放文件事件
                    {
                        char* dropped_file_path = event.drop.file;
                        printf("文件拖放事件: %s\n", dropped_file_path);
                        
                        // 加载并运行ROM
                        if (load_and_run_rom(&chip8, dropped_file_path)) {
                            rom_loaded = 1;
                            // 重置所有计时器
                            last_cycle_time = current_time;
                            last_timer_update = current_time;
                            cycle_accumulator = 0.0f;
                            timer_counter = 0;
                        } else {
                            printf("加载ROM失败，请检查文件格式和路径\n");
                        }
                        
                        // 释放SDL分配的文件路径内存
                        SDL_free(dropped_file_path);
                    }
                    break;
                    
                case SDL_WINDOWEVENT:
                    // 窗口事件处理
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        printf("窗口大小改变: %dx%d\n", event.window.data1, event.window.data2);
                    }
                    break;
            }
        }
        
        // 2. CPU执行（根据游戏速度控制）
        if (rom_loaded) {
            // 计算经过的时间（秒）
            Uint32 elapsed_ms = current_time - last_cycle_time;
            float elapsed_seconds = elapsed_ms / 1000.0f;
            
            // 根据游戏速度计算应该执行的CPU周期数
            // game_speed是每秒执行的指令数
            // elapsed_seconds是经过的时间（秒）
            // 所以应该执行的周期数 = 速度 * 时间
            float cycles_to_execute = game_speed * elapsed_seconds;
            
            // 累积到累加器中
            cycle_accumulator += cycles_to_execute;
            
            // 执行整数个CPU周期
            while (cycle_accumulator >= 1.0f) {
                chip8_cycle(&chip8);
                cycle_accumulator -= 1.0f;
                timer_counter++;
            }
            
            last_cycle_time = current_time;
            
            // 3. 定时器更新（固定60Hz）
            // 每16.67ms更新一次定时器（60Hz）
            if (current_time - last_timer_update >= 16) {  // 约60Hz
                // 更新定时器（60Hz）
                chip8_update_timers(&chip8);
                last_timer_update = current_time;
            }
        }
        
        // 4. 图形刷新（固定60Hz）
        static Uint32 last_graphics_update = 0;
        if (current_time - last_graphics_update >= 16) {  // 约60Hz
            if (chip8.draw_flag && rom_loaded) {
                chip8_graphics_update(&chip8);
                chip8.draw_flag = 0;
                frame_counter++;
                frame_count_since_last++;
                
                // 每60帧显示一次状态
                if (frame_counter % 60 == 0) {
                    printf("运行状态: 帧数=%d, PC=0x%03X, 声音定时器=%u, 游戏速度=%d指令/秒, 实际FPS=%.1f\n", 
                           frame_counter, chip8.pc, chip8.sound_timer, game_speed, current_fps);
                }
            }
            last_graphics_update = current_time;
            
            // 更新FPS显示
            update_fps_display();
        }
        
        // 5. 短暂延迟以避免过度占用CPU
        SDL_Delay(1);
    }
    
    // 清理资源
    printf("正在清理资源...\n");
    chip8_graphics_cleanup(&chip8);
    printf("模拟器已关闭\n");
    
    return 0;
}