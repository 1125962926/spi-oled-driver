#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>

#include "../def_spi_oled.h"
#include "include/parse_config.h"
#include "include/page.h"

/* 帧缓冲 */
char *oled_framebuffer = NULL;
/* 命令行参数 */
AppConfig config;
/* oled 设备文件描述符 */
int fd;
/* 帧缓冲大小 */
size_t buffer_size;

/*********************************** 信号处理 *********************************/
/**
 * @Description: 信号处理函数
 * @param {int} signum: 触发的信号
 * @return {*}
 */
void handle_signal(int signum) {
    printf("\n");
    if (signum == SIGINT) {
        printf("Caught SIGINT!");
    } else if (signum == SIGTERM) {
        printf("Caught SIGTERM!");
    }
    printf("\tCleaning up...\n");
    if (oled_framebuffer) {
        munmap(oled_framebuffer, buffer_size);
    }
    if (fd > 0) {
        close(fd);
    }
    exit(0);
}

/*********************************** 主函数 *********************************/
int main(int argc, char *argv[]) {
    int ret;

    /**************** 信号处理 *****************/
    /* 捕获 SIGTERM 常规终止信号（kill <PID>） */
    signal(SIGTERM, handle_signal);

    /* 捕获 SIGINT 中断信号（Ctrl + C） */
    signal(SIGINT, handle_signal);

    /**************** 解析命令行参数 *****************/
    /* 解析命令行参数 */ 
    config = parse_arguments(argc, argv);

    /* 检查是否传入 GPIO */
    if (config.oled_pins == NULL) {
        printf("\n****** Need oled_pins! Please retry! ******\n\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 打印配置信息 */ 
    if (config.verbose) {
        print_config(config);
    }

    /* 解析 GPIO */ 
    char *token = strtok(config.oled_pins, ",");
    int gpio_num = 0;
    int oled_gpios[PIN_NUM];
    while (token) {
        if (gpio_num >= PIN_NUM) {
            fprintf(stderr, "Error: Too many ports specified (max %d).\n", PIN_NUM);
            return 1;
        }
        oled_gpios[gpio_num++] = atoi(token);
        // 传入 NULL 表示继续从上次的位置查找下一个子字符串
        token = strtok(NULL, ",");
    }

    /**************** oled 设备配置 *****************/
    /* 打开设备 */
    fd = open("/dev/spi_oled", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    /* 尝试锁定文件 */ 
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        perror("flock");
        close(fd);
        return 1;
    }

    /* 使用 ioctl 传递 GPIO */
    struct oled_gpio_stuct gpio_group = {
        .scl_pin = oled_gpios[0],
        .mosi_pin = oled_gpios[1],
        .res_pin = oled_gpios[2],
        .dc_pin = oled_gpios[3]
    };
    ret = ioctl(fd, IOCTL_OLED_SET_GPIO, &gpio_group);
    if (ret < 0) {
        perror("ioctl failed: IOCTL_OLED_SET_GPIO");
        flock(fd, LOCK_UN);
        close(fd);
        return ret;
    }

    /**************** 内存映射 *****************/
    // 在用户空间可以通过 sysconf(_SC_PAGESIZE) 获取系统的页面大小：
    size_t page_size = sysconf(_SC_PAGESIZE);
    // printf("Page size: %ld\n", page_size); // 通常是 4096

    /* 计算映射需要的大小 */
    size_t num_pages = FRAME_BUFFER_SIZE / page_size;
    if (FRAME_BUFFER_SIZE % page_size != 0) {
        num_pages += 1; // 如果不是整数倍，增加一页
    }
    buffer_size = num_pages * page_size;

    /* 映射内存 */
    oled_framebuffer = mmap(NULL, buffer_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (oled_framebuffer == MAP_FAILED) {
        perror("Failed to mmap");
        flock(fd, LOCK_UN);
        close(fd);
        return ret;
    }

    // 主循环
    while (1) {
        int ret;
        /* 阻塞等待超时 */ 
        ret = poll(NULL, 0, config.interval);  // 使用 poll 实现定时器，并且休眠时不占用 CPU
        if (ret != 0) {
            perror("poll");
            break;
        }

        /* 设置帧缓冲数据 */
        // 只操作 oled_framebuffer 的前 1024 字节
        display_ui(config.page, oled_framebuffer, FRAME_BUFFER_SIZE);

        /* 刷新 oled */ 
        ret = ioctl(fd, IOCTL_OLED_REFRESH, &gpio_group);
        if (ret < 0) {
            perror("ioctl failed: IOCTL_OLED_REFRESH");
            break;
        }
    }
    
    munmap(oled_framebuffer, buffer_size);
    flock(fd, LOCK_UN);
    close(fd);
    return ret;
}