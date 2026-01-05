# ============ CHIP-8 模拟器 Makefile ============
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

# ============ 【核心修正】SDL2 精确路径 ============
# 你的SDL2根目录
SDL_DIR = D:/SDL2-devel-2.30.4-mingw/SDL2-2.30.4
# 头文件路径：指向 `include` 目录（里面有 SDL2 文件夹）
INC_PATH = -I$(SDL_DIR)/x86_64-w64-mingw32/include
# 库文件路径
LIB_PATH = -L$(SDL_DIR)/x86_64-w64-mingw32/lib

# ============ 编译与链接标志 ============
ALL_CFLAGS = $(CFLAGS) $(INC_PATH)
# 注意链接库顺序：-lmingw32 必须在最前
ALL_LDFLAGS = $(LIB_PATH) -lmingw32 -lSDL2main -lSDL2 -lm

# ============ 项目文件 ============
SRC_DIR = src
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/chip8.c
OBJ = $(SRC:.c=.o)
TARGET = chip8.exe

# ============ 构建规则 ============
all: $(TARGET)
	@echo "编译完成: $(TARGET)"
	@echo "请运行： .\$(TARGET)"

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(ALL_LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

run: $(TARGET)
	.\$(TARGET)

clean:
	del /f /q $(SRC_DIR)\*.o $(TARGET) 2>nul
	@echo 清理完成

.PHONY: all clean run