#!/bin/bash
###
 # @Author: Li RF
 # @Date: 2025-01-22 09:56:16
 # @LastEditors: Li RF
 # @LastEditTime: 2025-01-22 13:44:55
 # @Description: 
 # Email: 1125962926@qq.com
 # Copyright (c) 2025 Li RF, All Rights Reserved.
### 

# 读取 PID 文件
PID_FILE="./spi_oled_app.pid"

if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    echo "正在关闭 spi_oled_app (PID: $PID)..."
    kill "$PID"
    rm "$PID_FILE"  # 删除 PID 文件
    echo "spi_oled_app 已关闭。"
    # 等待程序运行一段时间，确保程序启动
    sleep 1
else
    echo "未找到 PID 文件，spi_oled_app 可能未运行。"
fi