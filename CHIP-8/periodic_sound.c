// periodic_sound.c - 周期性蜂鸣测试
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main() {
    FILE* file = fopen("beep_test.ch8", "wb");
    if (!file) return 1;
    
    // 指令序列
    uint16_t program[] = {
        0x600A,    // V0 = 10 (声音持续时间)
        0x610F,    // V1 = 15 (静音持续时间)
        0x6200,    // V2 = 0 (计数器)
        
        // 主循环开始 (地址0x206)
        0x3200,    // 如果 V2 == 0 则跳过
        0x1210,    // 跳转到 0x210
        
        // 播放声音
        0xF018,    // 声音定时器 = V0
        0x7001,    // V0 = V0 + 1 (逐渐延长声音)
        
        // 延迟循环
        0x6300,    // V3 = 0
        0xF315,    // 延迟定时器 = V3
        0xF307,    // V3 = 延迟定时器
        0x4300,    // 如果 V3 != 0 则跳过
        0x120E,    // 跳转到 0x20E
        
        // 静音
        0xF118,    // 声音定时器 = V1
        0x7101,    // V1 = V1 + 1 (逐渐延长静音)
        
        // 延迟循环
        0x6300,    // V3 = 0
        0xF315,    // 延迟定时器 = V3
        0xF307,    // V3 = 延迟定时器
        0x4300,    // 如果 V3 != 0 则跳过
        0x121A,    // 跳转到 0x21A
        
        // 切换状态
        0x8203,    // V2 = V2 XOR V0
        0x1206     // 跳回主循环开始
    };
    
    for (int i = 0; i < sizeof(program)/sizeof(program[0]); i++) {
        fputc(program[i] >> 8, file);
        fputc(program[i] & 0xFF, file);
    }
    
    fclose(file);
    printf("周期性蜂鸣测试ROM已创建\n");
    return 0;
}