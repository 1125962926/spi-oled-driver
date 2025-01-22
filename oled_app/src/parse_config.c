/*
 * @Author: Li RF
 * @Date: 2025-01-14 19:05:32
 * @LastEditors: Li RF
 * @LastEditTime: 2025-01-14 21:45:09
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "parse_config.h"

/**
 * @Description: 显示帮助信息
 * @param {char} *program_name: 程序名称
 * @return {*}
 */
void print_help(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -o, --oled_pins <scl,mosi,res,dc> Set oled pin number\n");
    printf("  -p, --page <number>               Set display page (1, 2, or 3)\n");
    printf("  -i, --interval <seconds>          Set update interval (default: 1)\n");
    printf("  -t, --text <string>               Set display text\n");
    printf("  -v, --verbose                     Enable verbose output\n");
    printf("  -h, --help                        Show this help message\n");
}

/**
 * @Description: 打印配置信息
 * @param {AppConfig} config: 配置信息
 * @return {*}
 */
void print_config(AppConfig config) {
    printf(" **************************\n");
    printf("Parse Information:\n");
    printf("    Display page: %d\n", config.page);
    printf("    Update Interval: %d millisecond(ms)\n", config.interval);
    printf("    Display Text: %s\n", config.text);
    printf("    GPIOs: %s\n", config.oled_pins);
}

/*
 * @description : 解析命令行参数
 * @param : 无
 * @return : 无
 */
AppConfig parse_arguments(int argc, char *argv[]) {
    AppConfig config = {
        .oled_pins = NULL,  // 默认为空字符串，提示用户传入
        .page = 1,          // 默认显示风格
        .interval = 1000,   // 默认更新间隔
        .text = "SPI OLED", // 默认显示文本
        .verbose = 0        // 默认关闭详细信息
    };

    /* 定义长选项 */ 
    struct option long_options[] = {
        {"oled_pins", required_argument, 0, 'o'},
        {"page",      required_argument, 0, 'p'},
        {"interval",  required_argument, 0, 'i'},
        {"text",      required_argument, 0, 't'},
        {"verbose",   no_argument,       0, 'v'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    /* 解析参数 */ 
    int opt;
    // 支持短选项和长选项
    // : 表示该选项需要一个参数，v 和 h 不需要
    // 如果解析到长选项，返回 val 字段的值（即第四列）
    while ((opt = getopt_long(argc, argv, "o:p:i:t:vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                config.oled_pins = optarg;
                break;
            case 'p':
                config.page = atoi(optarg);
                if (config.page < 1 || config.page > 3) {
                    fprintf(stderr, "Invalid page number. Use 1, 2, or 3.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                config.interval = atoi(optarg);
                if (config.interval <= 0) {
                    fprintf(stderr, "Interval must be a positive number.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                config.text = optarg;
                break;
            case 'v':
                config.verbose = 1;
                break;
            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    return config;
}
