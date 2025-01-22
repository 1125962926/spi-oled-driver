/*
 * @Author: Li RF
 * @Date: 2025-01-14 19:03:20
 * @LastEditors: Li RF
 * @LastEditTime: 2025-01-22 14:51:47
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef _DEF_SPI_OLED_H_
#define _DEF_SPI_OLED_H_


/* 设备信息 */
// SSD1306 的最大 SCLK 频率为 10 MHz，则每个 SCLK 周期为 100 ns
// 每个 SCLK 周期包括一个高电平和一个低电平，因此每个电平的持续时间应为 50 ns
#define DELAY_TIME_NS 0             /* 电平翻转延时，ns纳秒 */
#define SPI_OLED_CNT 1              /* 设备号个数 */
#define SPI_OLED_NAME "spi_oled"    /* 名字 */
/*  分辨率 128 x 64
    每一列有8字节（64 / 8）数据，共有128行
	x坐标：0~127
    y坐标：0~63 */
#define FRAME_WIDTH 128
#define FRAME_HEIGHT 64
#define FRAME_BUFFER_SIZE (FRAME_WIDTH * FRAME_HEIGHT / 8)

/* gpio 申请标志对应 BIT */
enum {
    SCL_BIT = 0,
    MOSI_BIT = 1,
    RES_BIT = 2,
    DC_BIT = 3
};

/* ioctl 指令集 */
enum {
    UNUSE = 0x00,
    IOCTL_OLED_SET_GPIO = 0x01,
    IOCTL_OLED_OPEN = 0x02,
    IOCTL_OLED_CLOSE = 0x03,
    IOCTL_OLED_REFRESH = 0x04,
    IOCTL_OLED_CLEAR = 0x05
};

/* gpio 电平 */
enum {
    GPIO_LOW = 0,
    GPIO_HIGH = 1
};

/* DC 引脚控制指令 */
enum {
    OLED_CMD = 0x00,
    OLED_DATA = 0x01
};

/* oled 引脚结构体 */
struct oled_gpio_stuct{
    int scl_pin;
    int mosi_pin;
    int res_pin;
    int dc_pin;
};
#define PIN_NUM ((int)(sizeof(struct oled_gpio_stuct) / sizeof(int)))




#endif /* _DEF_SPI_OLED_H_ */
