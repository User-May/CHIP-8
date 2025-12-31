// 文件：create_rom.c
#include <stdio.h>
#include <stdint.h>

int main() {
    const char* filename = "test_real.ch8";
    
    // 一个有效的CHIP-8测试程序（全用十六进制值表示）
    uint8_t program[] = {
        0x00, 0xE0, // 00E0: 清屏指令 (CLS)
        0x61, 0x02, // 6102: 设置寄存器 V1 = 0x02
        0x62, 0x03, // 6203: 设置寄存器 V2 = 0x03
        0x81, 0x24, // 8124: 相加 V1 = V1 + V2
        0x12, 0x00  // 1200: 跳转到地址 0x200 (JP 0x200，无限循环)
    };
    
    FILE* file = fopen(filename, "wb"); // 注意 "wb" 表示写二进制文件
    if (!file) {
        perror("打开文件失败");
        return 1;
    }
    
    size_t written = fwrite(program, sizeof(uint8_t), sizeof(program), file);
    fclose(file);
    
    if (written == sizeof(program)) {
        printf("成功创建ROM文件: %s\n", filename);
        printf("大小: %zu 字节\n", sizeof(program));
        
        // 可选：打印内容以便核对
        printf("内容 (十六进制): ");
        for (size_t i = 0; i < sizeof(program); i++) {
            printf("%02X ", program[i]);
        }
        printf("\n");
        return 0;
    } else {
        printf("写入文件失败。\n");
        return 1;
    }
}