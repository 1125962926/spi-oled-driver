/*
 * @Author: Li RF
 * @Date: 2025-01-14 22:13:40
 * @LastEditors: Li RF
 * @LastEditTime: 2025-01-22 10:11:43
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef _PAGE_H_
#define _PAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../def_spi_oled.h"

typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;

/* 字体大小 */
enum {
    FONT_12 = 12,
    FONT_16 = 16,
    FONT_24 = 24
};

void display_ui(int page, char *frame_buffer, size_t frame_size);

#endif