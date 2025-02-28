/*
 * @Author: Li RF
 * @Date: 2025-01-14 19:15:24
 * @LastEditors: Li RF
 * @LastEditTime: 2025-02-28 13:20:36
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#include <time.h>

#include "page.h"
#include "oledfont.h"

static char *buffer; // 缓冲区
static size_t size; // 缓冲区大小
/***************************** 基础操作 ******************************/
/**
 * @Description: 在OLED屏幕中绘制点
 * @param {uint8_t} x: x 坐标
 * @param {uint8_t} y: y 坐标
 * @param {uint8_t} point: 1 填充 0,清空
 * @return {*}
 */
// oled 单字节的第一行是最高位，最后一行是最低位（0x80 绘制第一行）
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t point)
{
    uint8_t pos, bx, temp = 0;
    uint8_t *page_start;
    
    if (x >= FRAME_WIDTH || y >= FRAME_HEIGHT)
        return;

    // 计算页号（从上到下共 8 页）
    pos = y / 8;

    // 页内的当前行（从上到下共 8 行，一个字节）
    bx = y % 8;
    
    // 将该行对应的位设置为 1
    temp = 1 << (7 - bx);

    // 计算该页在缓冲区的起始位置
    page_start = (uint8_t*)(buffer + pos * FRAME_WIDTH);

    if (point)
        page_start[x] |= temp;
    else
        page_start[x] &= ~temp;
}

/**
 * @Description: 显示单个英文字符
 * @param {uint8_t} x: 字符显示位置的起始x坐标
 * @param {uint8_t} y: 字符显示位置的起始y坐标
 * @param {uint8_t} chr: 显示字符的ascii码（0～94）
 * @param {uint8_t} size: 选择字体 12/16/24
 * @param {uint8_t} point: 1 填充 0,清空
 * @return {*}
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t point)
{
    uint8_t temp, t, t1;
    uint8_t y0 = y;
    // 得到字体一个字符对应点阵集所占的字节数
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); 
    // 得到偏移后的值
    chr = chr - ' ';
    for (t = 0; t < csize; t++)
    {
        if (size == FONT_12)
            temp = asc2_1206[chr][t]; // 调用1206字体
        else if (size == FONT_16)
            temp = asc2_1608[chr][t]; // 调用1608字体
        else if (size == FONT_24)
            temp = asc2_2412[chr][t]; // 调用2412字体
        else
            return; // 没有的字库
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                OLED_DrawPoint(x, y, point);
            else
                OLED_DrawPoint(x, y, !point);
            temp <<= 1;
            y++;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @Description: 显示中文字符串
 * @param {uint8_t} x: 汉语字符串的起始x坐标
 * @param {uint8_t} y: 汉语字符串的起始y坐标
 * @param {uint8_t} index: 二维字库中的列向量，第几个字
 * @param {uint8_t} size: 中文字符串的大小
 * @param {uint8_t} point: 1 填充 0,清空
 * @return {*}
 */
void OLED_Chinese_Text(uint8_t x, uint8_t y, uint8_t index, uint8_t size, uint8_t point)
{
    uint8_t temp, t, t1;
    uint8_t y0 = y;
    // 得到字体一个字符对应点阵集所占的字节数
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * size;
    for (t = 0; t < csize; t++)
    {
        temp = Chinese_Text[index][t];
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                OLED_DrawPoint(x, y, point);
            else
                OLED_DrawPoint(x, y, !point);
            temp <<= 1;
            y++;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @Description: 显示数字
 * @param {uint8_t} x: 数字的起始x坐标
 * @param {uint8_t} y: 数字的起始y坐标
 * @param {uint32_t} num: 数字（0～4294967295）
 * @param {uint8_t} len: 显示数字的长度
 * @param {uint8_t} size: 显示数字的大小
 * @return {*}
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0; // 用于控制是否显示前导零。如果 enshow 为 0 且当前位是前导零，则显示空格
    uint32_t pow_val = 1; // 用于存储当前位的权重（10的幂次）

    // 预先计算最高位的权重（10^(len-1)）
    for (t = 0; t < len - 1; t++) {
        pow_val *= 10;
    }

    for (t = 0; t < len; t++) {
        temp = (num / pow_val) % 10; // 获取当前位的数字
        pow_val /= 10; // 更新 pow_val 为下一次计算做准备

        // 处理前导零
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                OLED_ShowChar(x + (size / 2) * t, y, ' ', size, 1);
                continue;
            } else {
                enshow = 1; // 遇到非零数字后，允许显示后续的零
            }
        }

        // 显示当前位的数字
        OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size, 1);
    }
}
/***************************** 区域操作 ******************************/
/**
 * @Description: 显示白色背景
 * @param {char} *buffer: 缓冲区
 * @param {size_t} size: 缓冲区大小
 * @return {*}
 */
void OLED_display_white(void) {
    memset(buffer, 0xFF, size);
}
/**
 * @Description: 清空
 * @param {char} *buffer: 缓冲区
 * @param {size_t} size: 缓冲区大小
 * @return {*}
 */
void OLED_Clear(void) {
    memset(buffer, 0x00, size);
}
/**
 * @Description: 填充指定区域
 * @param {uint8_t} x1: 起始x坐标
 * @param {uint8_t} y1: 起始y坐标
 * @param {uint8_t} x2: 结束x坐标
 * @param {uint8_t} y2: 结束y坐标
 * @param {uint8_t} point: 1 填充 0,清空
 * @return {*}
 */
void OLED_Fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t point)
{
    uint8_t x, y;
    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
            OLED_DrawPoint(x, y, point);
    }
}

/**
 * @Description: 显示显示BMP图片
 * @param {uint8_t} x1: 起始x坐标
 * @param {uint8_t} y1: 起始y坐标
 * @param {uint8_t} page: 图片帧号，第几帧
 * @param {uint8_t} image_x: 图像长
 * @param {uint8_t} image_y: 图像宽
 * @return {*}
 */
void OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t page, uint8_t image_x, uint8_t image_y)
{
    uint8_t point_byte; /* 图像数组每一字节 */
    uint8_t t2, t1;
    uint8_t y0 = y;
    // 得到每一帧所占的字节数
    uint8_t csize = (image_y / 8 + ((image_y % 8) ? 1 : 0)) * image_x;

    for (t2 = 0; t2 < csize; t2++) /* 按字节写入 */
    {
        point_byte = GIF_image[page][t2];
        for (t1 = 0; t1 < 8; t1++) /* 字节内每一像素点 */
        {
            if (point_byte & 0x80)
                OLED_DrawPoint(x, y, 1);
            else
                OLED_DrawPoint(x, y, 0);
            point_byte <<= 1;
            y++;
            if ((y - y0) == image_y)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @Description: 显示英文字符串
 * @param {uint8_t} x: 字符串的起始x坐标
 * @param {uint8_t} y: 字符串的起始y坐标
 * @param {uint8_t} *p: 字符串起始地址
 * @param {uint8_t} size: 显示字符的大小
 * @return {*}
 */
void OLED_ShowString(uint8_t x, uint8_t y, const uint8_t *p, uint8_t size)
{
    while ((*p <= '~') && (*p >= ' ')) // 判断是不是非法字符!
    {
        if (x > (FRAME_WIDTH - (size / 2)))
        {
            x = 0;
            y += size;
        }
        if (y > (FRAME_HEIGHT - size))
        {
            y = x = 0;
            OLED_Clear();
        }
        OLED_ShowChar(x, y, *p, size, 1);
        x += size / 2;
        p++;
    }
}

/***************************** UI 组件 ******************************/
/**
 * @Description: 获取当前时间
 * @param {char} *date_t: 日期字符串
 * @param {char} *time_t: 时间字符串
 * @return {*}
 */
void get_current_time(char *date_str, char *time_str, size_t size1, size_t size2) 
{
    time_t current_time;
    struct tm* timeinfo;

    // 获取当前时间的时间戳
    time(&current_time);
    // 将时间戳转换为本地时间
    timeinfo = localtime(&current_time);
    if (timeinfo == NULL) {
        fprintf(stderr, "Error: Failed to convert time to local time.\n");
        return;
    }

	// 格式化日期
    strftime(date_str, size1, "%Y-%m-%d", timeinfo);
    // 格式化时间
    strftime(time_str, size2, "%H:%M:%S", timeinfo);
}

/**
 * @Description: 获取 CPU 占用率
 * @return {*}
 */
int get_cpu_usage(char *cpu_usage, size_t size) {
    // 使用mpstat命令获取CPU占用率，popen 打开的文件会将命令的输出保留在一个缓冲区中
    // mpstat 1 1 表示每秒采样一次，共采样一次
    // /all/：匹配包含 all 的行，$NF 表示最后一列（即 %idle，CPU 空闲率），head -n 1 限制只输出第一行
    FILE *fp = popen("mpstat 1 1 | awk '/all/ {print 100 - $NF}' | head -n 1", "r");
    if (fp == NULL) {
        perror("Failed to run mpstat command");
        return -1;
    }
    // 读取命令输出
    char temp[size];
    if (fgets(temp, size, fp) == NULL) {
        perror("Failed to read CPU usage");
        pclose(fp);
        return -1;
    }
    pclose(fp);
    // 添加前缀
    snprintf(cpu_usage, size, "CPU: %.2f%%", atof(temp));
    // 查找字符串中的换行符并将其替换为终止符 \0，确保字符串没有多余的换行符
    cpu_usage[strcspn(cpu_usage, "\n")] = '\0';
    return 0;
}

/**
 * @Description: 获取 CPU 频率
 * @return {*}
 */
int get_cpu_frequency(char *cpu_freq, size_t size) {
    FILE *file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
    if (file == NULL) {
        perror("Failed to open CPU frequency file");
        return -1;
    }
    char line[size];
    if (fgets(line, size, file) == NULL) {
        perror("Failed to read CPU frequency");
        fclose(file);
        return -1;
    }
    fclose(file);
    // 添加前缀
    snprintf(cpu_freq, size, "CPU: %dMHz", atoi(line) / 1000);
    return 0;
}

/**
 * @Description: 获取 GPU 占用率
 * @return {*}
 */
int get_gpu_usage(char *gpu_usage, size_t size) {
    FILE *file = fopen("/sys/class/devfreq/fb000000.gpu/load", "r");
    if (file == NULL) {
        perror("Failed to open GPU load file");
        return -1;
    }

    char line[size];
    if (fgets(line, size, file) == NULL) {
        perror("Failed to read GPU load");
        fclose(file);
        return -1;
    }
    fclose(file);
    // 提取@前的数字
    char *at_pos = strchr(line, '@');
    if (at_pos == NULL) {
        fprintf(stderr, "Invalid GPU load format\n");
        return -1;
    }
    *at_pos = '\0'; // 截断字符串，保留@前的部分
    // 添加前缀
    snprintf(gpu_usage, size, "GPU: %d%%", atoi(line));
    return 0;
}

/**
 * @Description: 获取 NPU 占用率
 * @return {*}
 */
int get_npu_usage(char *npu_usage, size_t size) {
    FILE *file = fopen("/sys/kernel/debug/rknpu/load", "r");
    if (file == NULL) {
        perror("Failed to open NPU load file");
        return -1;
    }
    // 读取文件内容
    char line[size];
    if (fgets(line, size, file) == NULL) {
        perror("Failed to read NPU load file");
        fclose(file);
        return -1;
    }
    fclose(file);
    // 添加前缀
    snprintf(npu_usage, size, "NPU: ");
    // 使用 strchr 查找 % 并提取数字
    char *ptr = line;
    int core_count = 0;
    while ((ptr = strchr(ptr, '%')) != NULL) {
        char *num_start = ptr - 1;
        // 回溯直到找到数字的起始位置
        while (num_start >= line && isdigit(*(num_start - 1))) {
            num_start--;
        }
        // 提取数字，atoi 会从给定的字符串起始位置开始解析，直到遇到第一个非数字字符为止（%前的所有数字）
        if (isdigit(*num_start)) {
            int value = atoi(num_start);
            if (value >= 0 && value <= 100) {
                // 将数字追加到结果字符串中
                char buffer[4]; // 最多三位数字
                snprintf(buffer, sizeof(buffer), "%d%%", value);
                strcat(npu_usage, buffer);
                if (core_count >= 0 && core_count < 2) {
                    strcat(npu_usage, " "); // 添加空格分隔
                }
                core_count++;
            } else {
                // 如果数字不在 0-100 范围内，跳过
                fprintf(stderr, "Invalid NPU load value: %d (must be 0-100)\n", value);
            }
        } else {
            // 如果 num_start 指向的位置不是数字字符，跳过
            fprintf(stderr, "Invalid NPU load format: expected a number before %%\n");
        }
        ptr++; // 继续查找下一个 %
    }
    return 0;
}

/**
 * @Description: 获取芯片温度
 * @return {*}
 */
int get_temperature(char *temperature, size_t size) {
    // 使用sensors命令获取温度
    FILE *fp = popen("sensors | awk '/temp1/ {print $2}' | head -n 1", "r");
    if (fp == NULL) {
        perror("Failed to run sensors command");
        return -1;
    }
    // 读取命令输出
    char line[size];
    if (fgets(line, size, fp) == NULL) {
        perror("Failed to read chip temperature");
        pclose(fp);
        return -1;
    }
    pclose(fp);
    // 添加前缀
    snprintf(temperature, size, "Temper: %s", line);
    // 去掉换行符
    temperature[strcspn(temperature, "\n")] = '\0';
    return 0;
}

/***************************** UI 界面 ******************************/

/**
 * @Description: 时间屏保
 * @return {*}
 */
void display_style_1(void) {
    char date_str[20];
    char time_str[20];
    // 获取当前时间
    get_current_time(date_str, time_str, sizeof(date_str), sizeof(time_str));
    OLED_ShowString(0, 30, (uint8_t *)date_str, FONT_12);
    OLED_ShowString(0, 40, (uint8_t *)time_str, FONT_24);
}

/**
 * @Description: 硬件占用率
 * @return {*}
 */
void display_style_2(void) {
    // 初始化缓冲区
    char cpu_usage[20] = {0}; // 过滤后的值长度最大为 6 （100.00）
    char cpu_freq[20] = {0};
    char gpu_usage[20] = {0}; // 指令返回的字符串长度为 16
    char npu_usage[50] = {0}; // 指令返回的字符串长度为 45
    char temperature[20] = {0};
    int ret;

    // 获取CPU占用率
    ret = get_cpu_usage(cpu_usage, sizeof(cpu_usage));
    if (ret != 0) {
        printf("Failed to get CPU usage\n");
    }
    // 获取CPU频率
    // ret = get_cpu_frequency(cpu_freq, sizeof(cpu_freq));
    // if (ret != 0) {
    //     printf("Failed to get CPU frequency\n");
    // }
    // 获取GPU占用率
    ret = get_gpu_usage(gpu_usage, sizeof(gpu_usage));
    if (ret != 0) {
        printf("Failed to get GPU usage\n");
    }
    // 获取NPU占用率
    ret = get_npu_usage(npu_usage, sizeof(npu_usage));
    if (ret != 0) {
        printf("Failed to get NPU usage\n");
    }
    // 获取芯片温度
    ret = get_temperature(temperature, sizeof(temperature));
    if (ret != 0) {
        printf("Failed to get chip temperature\n");
    }

    // 显示
    OLED_ShowString(0, 0, (uint8_t *)cpu_usage, FONT_16);
    // OLED_ShowString(64, 0, (uint8_t *)cpu_freq, FONT_16);
    OLED_ShowString(0, 16, (uint8_t *)gpu_usage, FONT_16);
    OLED_ShowString(0, 32, (uint8_t *)npu_usage, FONT_16);
    OLED_ShowString(0, 48, (uint8_t *)temperature, FONT_16);

}

void display_style_3(void) {
    printf("Displaying Style 3: Animated Graphics\n");
}


/***************************** 选择菜单 ******************************/
/**
 * @Description: 根据用户选择显示不同的界面
 * @param {char} *buffer: 缓冲区
 * @param {size_t} size: 缓冲区大小
 * @return {*}
 */
void display_ui(int page, char *frame_buffer, size_t frame_size) {
    /* 保存参数 */
    buffer = frame_buffer;
    size = frame_size;

    OLED_Clear(); // 清空屏幕缓冲
    switch (page) {
        case 1:
            display_style_1();
            break;
        case 2:
            display_style_2();
            break;
        case 3:
            display_style_3();
            break;
        default:
            printf("Invalid page number\n");
            break;
    }
}
