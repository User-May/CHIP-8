# CHIP-8 模拟器 Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
TARGET = chip8.exe

# 源文件
SRC = src/main.c src/chip8.c
OBJ = $(SRC:.c=.o)

# 默认目标
all: $(TARGET) test.ch8
	@echo "编译完成: $(TARGET)"
	@echo "测试ROM: test.ch8"
	@echo "使用: .\$(TARGET)"

# 链接主程序
$(TARGET): $(OBJ)
	$(CC) $^ -o $@

# 编译规则
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 创建测试ROM (直接内嵌在Makefile中)
test.ch8:
	@echo "创建测试ROM..."
	@echo -ne "\x00\xE0\x61\x02\x62\x03\x81\x24\x12\x00" > test.ch8
	@echo "测试ROM创建完成: test.ch8 (10字节)"

# 或者使用一个C程序创建
create_rom: create_rom.c
	$(CC) create_rom.c -o create_rom.exe
	.\create_rom.exe

# 清理
clean:
	del /f /q src\*.o $(TARGET) test.ch8 create_rom.exe 2>nul
	@echo 清理完成

# 运行
run: $(TARGET) test.ch8
	.\$(TARGET)

.PHONY: all clean run create_rom