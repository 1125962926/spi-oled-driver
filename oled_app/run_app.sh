#!/bin/bash
###
 # @Author: Li RF
 # @Date: 2025-01-15 22:11:46
 # @LastEditors: Li RF
 # @LastEditTime: 2025-01-22 16:49:51
 # @Description: 
 # Email: 1125962926@qq.com
 # Copyright (c) 2025 Li RF, All Rights Reserved.
### 

# 启用 set -e，任何命令失败时脚本立即退出
set -e

# 提取调用脚本时的参数，获取路径
script_path=$(dirname "$0")

# SPI 接口引脚定义
scl_pin=98
mosi_pin=101
res_pin=100
dc_pin=99
pin_str="$scl_pin,$mosi_pin,$res_pin,$dc_pin"

# PID 文件路径
PID_FILE="${script_path}/spi_oled_app.pid"

# 检查 PID 文件是否存在
if [ -f "$PID_FILE" ]; then
#    echo "APP is already running. Exiting."
    exit 1
fi

sleep 2

# 后台执行脚本
${script_path}/spi_oled_app -p 2 -o "$pin_str" &

# 获取后台进程的 PID
APP_PID=$!

# 等待程序运行一段时间，确保程序启动
sleep 1

# 检查进程是否仍在运行
if kill -0 "$APP_PID" > /dev/null 2>&1; then
    echo "程序运行正常，保存 PID: $APP_PID"
    echo "$APP_PID" > "$PID_FILE"
else
    echo "程序启动失败，不保存 PID。"
    exit 1
fi


# debug
# gdb --args ./spi_oled_app -o 98,101,100,99
