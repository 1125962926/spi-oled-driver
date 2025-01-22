/*
 * @Author: Li RF
 * @Date: 2025-01-14 19:05:32
 * @LastEditors: Li RF
 * @LastEditTime: 2025-01-16 20:16:44
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef _PARSE_CONFIG_H_
#define _PARSE_CONFIG_H_


/* 定义命令行参数结构体 */ 
typedef struct {
    char *oled_pins; // 控制引脚
    int page;        // 显示主页
    int interval;    // 更新间隔（ms毫秒）
    char *text;      // 显示文本
    int verbose;     // 是否显示详细信息
} AppConfig;


void print_help(const char *program_name);
AppConfig parse_arguments(int argc, char *argv[]);
void print_config(AppConfig config);

#endif
