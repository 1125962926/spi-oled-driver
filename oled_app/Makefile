# 最终可执行文件
TARGET = spi_oled_app


# 源文件目录
SRC_DIR = src
# include 路径
INC_DIR ?= include
# 目标文件目录
OBJ_DIR = obj


# 编译器
CC = gcc
# 编译选项
CFLAGS = -Wall -g
CFLAGS += -I$(INC_DIR)


# 获取所有源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
SRCS += main.c
# 生成对应的目标文件列表（修改后缀）
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))


# 默认目标
all: $(OBJ_DIR) $(TARGET)

# 创建目标文件目录
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

# 生成目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# 伪目标
.PHONY: all clean