/*
 * @Author: Li RF
 * @Date: 2025-01-12 20:35:44
 * @LastEditors: Li RF
 * @LastEditTime: 2025-01-22 15:09:29
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/fcntl.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include "def_spi_oled.h"

/***************************** oled 设备 ******************************/
/* spi_oled 设备结构体 */
struct spi_oled_device{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
    char *frame_buffer;     /* 帧缓冲区 */
    struct oled_gpio_stuct gpio_group; /* gpio 序号 */
    unsigned long gpio_request_flag;   /* gpio 申请标志 */
};
struct spi_oled_device spi_oled_dev; /* oled 设备 */
size_t buffer_size; /* 帧缓冲区大小（可能会被修正，所以使用全局变量） */

/***************************** spi 接口 ******************************/
/**
 * @Description: 软件模拟 spi 写
 * @param {uint8_t} data: 待写入数据
 * @return {*}
 */
static void spi_write_byte(uint8_t data) {
    // 逐位发送数据
    for (int i = 0; i < 8; i++)
    {
        // 产生时钟下降沿
        gpio_set_value(spi_oled_dev.gpio_group.scl_pin, GPIO_LOW);
        ndelay(DELAY_TIME_NS);  // 延时

        // 传输最高位
        gpio_set_value(spi_oled_dev.gpio_group.mosi_pin, (data & 0x80));
        ndelay(DELAY_TIME_NS);

        // 产生时钟上升沿
        gpio_set_value(spi_oled_dev.gpio_group.scl_pin, GPIO_HIGH);
        ndelay(DELAY_TIME_NS);

        // 左移更新最高位
        data = data << 1;
    }
}

/**
 * @Description: oled 写字节
 * @param {uint8_t} data: 待写入数据
 * @param {uint8_t} cmd: 命令或数据
 * @return {*}
 */
static void oled_write_byte(uint8_t data, uint8_t cmd)
{
    /* DC 引脚，指令或数据 */
    gpio_set_value(spi_oled_dev.gpio_group.dc_pin, cmd);
    
    /* 调用 spi 写字节 */
    spi_write_byte(data);
    
    /* DC 引脚，传输完成保持高 */
    gpio_set_value(spi_oled_dev.gpio_group.dc_pin, GPIO_HIGH);
}

/***************************** GPIO 配置 ******************************/
/**
 * @description : 检查 GPIO 是否申请并配置
 * @param : 无
 * @return : 无
 */
static bool oled_gpio_check(void) {
    if (!test_bit(SCL_BIT, &spi_oled_dev.gpio_request_flag)) {
        printk(KERN_ERR "%s: SCL GPIO not set\n", SPI_OLED_NAME);
        return false;
    }
    if (!test_bit(MOSI_BIT, &spi_oled_dev.gpio_request_flag)) {
        printk(KERN_ERR "%s: MOSI GPIO not set\n", SPI_OLED_NAME);
        return false;
    }
    if (!test_bit(RES_BIT, &spi_oled_dev.gpio_request_flag)) {
        printk(KERN_ERR "%s: RES GPIO not set\n", SPI_OLED_NAME);
        return false;
    }
    if (!test_bit(DC_BIT, &spi_oled_dev.gpio_request_flag)) {
        printk(KERN_ERR "%s: DC GPIO not set\n", SPI_OLED_NAME);
        return false;
    }
    return true;
}
/**
 * @description : 释放 GPIO，清除 GPIO 申请标志
 * @param : 无
 * @return : 无
 */
static void oled_gpio_free(void){

    /* SCL */
    if (test_bit(SCL_BIT, &spi_oled_dev.gpio_request_flag))
    {
        gpio_free(spi_oled_dev.gpio_group.scl_pin);
        clear_bit(SCL_BIT, &spi_oled_dev.gpio_request_flag);
    }
    
    /* MOSI */
    if (test_bit(MOSI_BIT, &spi_oled_dev.gpio_request_flag))
    {
        gpio_free(spi_oled_dev.gpio_group.mosi_pin);
        clear_bit(MOSI_BIT, &spi_oled_dev.gpio_request_flag);
    }
        
    /* RES */
    if (test_bit(RES_BIT, &spi_oled_dev.gpio_request_flag))
    {
        gpio_free(spi_oled_dev.gpio_group.res_pin);
        clear_bit(RES_BIT, &spi_oled_dev.gpio_request_flag);
    }
        
    /* DC */
    if (test_bit(DC_BIT, &spi_oled_dev.gpio_request_flag))
    {
        gpio_free(spi_oled_dev.gpio_group.dc_pin);
        clear_bit(DC_BIT, &spi_oled_dev.gpio_request_flag);
    }
    
}

/**
 * @description : 初始化 GPIO
 * @param : 无
 * @return : 无
 */
static int oled_gpio_init(void) {
    int ret;

    // 申请 scl
    ret = gpio_request(spi_oled_dev.gpio_group.scl_pin, "scl");
    if (ret) {
        printk(KERN_ERR "%s: Failed to request SCL GPIO\n", SPI_OLED_NAME);
        goto err;
    }
    set_bit(SCL_BIT, &spi_oled_dev.gpio_request_flag);

    // 申请 mosi
    ret = gpio_request(spi_oled_dev.gpio_group.mosi_pin, "mosi");
    if (ret) {
        printk(KERN_ERR "%s: Failed to request MOSI GPIO\n", SPI_OLED_NAME);
        goto err;
    }
    set_bit(MOSI_BIT, &spi_oled_dev.gpio_request_flag);

    // 申请 res 
    ret = gpio_request(spi_oled_dev.gpio_group.res_pin, "res");
    if (ret) {
        printk(KERN_ERR "%s: Failed to request RES GPIO\n", SPI_OLED_NAME);
        goto err;
    }
    set_bit(RES_BIT, &spi_oled_dev.gpio_request_flag);

    // 申请 dc 
    ret = gpio_request(spi_oled_dev.gpio_group.dc_pin, "dc");
    if (ret) {
        printk(KERN_ERR "%s: Failed to request DC GPIO\n", SPI_OLED_NAME);
        goto err;
    }
    set_bit(DC_BIT, &spi_oled_dev.gpio_request_flag);

    // 配置 GPIO 方向，默认电平
    gpio_direction_output(spi_oled_dev.gpio_group.scl_pin, GPIO_HIGH);// 空闲时高电平
    gpio_direction_output(spi_oled_dev.gpio_group.mosi_pin, GPIO_HIGH);// 空闲时高电平
    gpio_direction_output(spi_oled_dev.gpio_group.res_pin, GPIO_HIGH);// 初始为高电平。拉低 100ms 后拉高，执行 reset
    gpio_direction_output(spi_oled_dev.gpio_group.dc_pin, OLED_DATA);// DC 初始为高电平

    return 0;

err:
    oled_gpio_free();
    return ret;
}

/***************************** OLED 初始化 ******************************/
/**
 * @description : OLED 初始化
 * @param : 无
 * @return : 无
 */
static void oled_start_init(void) {

    /* 拉低 RES 引脚 100 ms，再拉高，完成复位 */
	gpio_set_value(spi_oled_dev.gpio_group.res_pin, GPIO_LOW);
	mdelay(100);
	gpio_set_value(spi_oled_dev.gpio_group.res_pin, GPIO_HIGH);

    oled_write_byte(0xAE, OLED_CMD); // 关闭显示 DCDC OFF
    oled_write_byte(0xD5, OLED_CMD); // 设置时钟分频因子,震荡频率
    oled_write_byte(80, OLED_CMD);   //[3:0],分频因子;[7:4],震荡频率
    oled_write_byte(0xA8, OLED_CMD); // 设置驱动路数
    oled_write_byte(0X3F, OLED_CMD); // 默认0X3F(1/64)
    oled_write_byte(0xD3, OLED_CMD); // 设置显示偏移
    oled_write_byte(0X00, OLED_CMD); // 默认为0

    oled_write_byte(0x40, OLED_CMD); // 设置显示开始行 [5:0],行数.

    oled_write_byte(0x8D, OLED_CMD); // 电荷泵设置，DCDC 命令
    oled_write_byte(0x14, OLED_CMD); // DCDC ON
    oled_write_byte(0x20, OLED_CMD); // 设置内存地址模式
    oled_write_byte(0x02, OLED_CMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
    oled_write_byte(0xA1, OLED_CMD); // 段重定义设置,bit0:0,0->0;1,0->127;
    oled_write_byte(0xC0, OLED_CMD); // 设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
    oled_write_byte(0xDA, OLED_CMD); // 设置COM硬件引脚配置
    oled_write_byte(0x12, OLED_CMD); //[5:4]配置

    oled_write_byte(0x81, OLED_CMD); // 对比度设置
    oled_write_byte(0xEF, OLED_CMD); // 1~255;默认0X7F (亮度设置,越大越亮)
    oled_write_byte(0xD9, OLED_CMD); // 设置预充电周期
    oled_write_byte(0xf1, OLED_CMD); //[3:0],PHASE 1;[7:4],PHASE 2;
    oled_write_byte(0xDB, OLED_CMD); // 设置VCOMH 电压倍率
    oled_write_byte(0x30, OLED_CMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

    oled_write_byte(0xA4, OLED_CMD); // 全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
    oled_write_byte(0xA6, OLED_CMD); // 设置显示方式;bit0:1,反相显示;0,正常显示
    oled_write_byte(0xAF, OLED_CMD); // 开启显示

}

/***************************** OLED 控制函数 ******************************/
//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	单页
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 	
/* 屏幕每一列有8字节（64 / 8）数据，共有128列 */
/* 每一字节行（8行），称为一页，共 8 页 */
/* 每页的刷新方向为每一字节从上至下，然后每一行从左至右 */ 
/**
 * @description : 刷新 OLED
 * @param : 无
 * @return : 无
 */
static void refresh_oled(void) {
    uint8_t *page_start;
    // 每一页
    for (uint8_t i = 0; i < FRAME_HEIGHT / 8; i++) 
    {
        oled_write_byte(0xb0 + i, OLED_CMD); // 设置页地址（0~7）
        oled_write_byte(0x00, OLED_CMD);     // 设置显示位置—列低地址
        oled_write_byte(0x10, OLED_CMD);     // 设置显示位置—列高地址

        // 计算当前页的起始位置，（0，0）在左上角
        // page_start = spi_oled_dev.frame_buffer + i * FRAME_WIDTH;
        // 反转页顺序，如果（0，0）在左下角，则需要先读取最后一页，再读取倒数第二页
        page_start = spi_oled_dev.frame_buffer + (FRAME_HEIGHT / 8 - 1 - i) * FRAME_WIDTH;

        // 确保不超出 1024 字节的范围
        if ((i + 1) * FRAME_WIDTH > FRAME_BUFFER_SIZE) {
            break;
        }

        // 写入当前页的所有字节（从左到右共 128 字节）
        for (uint8_t n = 0; n < FRAME_WIDTH; n++)
        {
            oled_write_byte(page_start[n], OLED_DATA);
        }
    }
}

/**
 * @description : 开启 OLED
 * @param : 无
 * @return : 无
 */
static void open_oled(void) {
    oled_write_byte(0X8D, OLED_CMD); // SET DCDC命令
    oled_write_byte(0X14, OLED_CMD); // DCDC ON
    oled_write_byte(0XAF, OLED_CMD); // DISPLAY ON
}

/**
 * @description : 关闭 OLED
 * @param : 无
 * @return : 无
 */
static void close_oled(void) {
    oled_write_byte(0X8D, OLED_CMD); // SET DCDC命令
    oled_write_byte(0X10, OLED_CMD); // DCDC OFF
    oled_write_byte(0XAE, OLED_CMD); // DISPLAY OFF
}

/**
 * @description : OLED 清屏
 * @param : 无
 * @return : 无
 */
static void clear_oled(void) {

	memset(spi_oled_dev.frame_buffer, 0, buffer_size); 
	refresh_oled();//更新显示
}


/***************************** 字符设备操作集 ******************************/
/**
 * @Description: open 函数
 * @param {inode} *inode: 
 * @param {file} *file: 
 * @return {*}
 */
static int oled_open(struct inode *inode, struct file *file) {

    // 如果是第一次打开设备文件，这里会跳过，等待 ioctl 的初始化
    // 如果已经设置过 GPIO，则进行 GPIO 的初始化
    // 因为每次关闭设备文件，会进行 GPIO 的释放，防止占用
    if(spi_oled_dev.gpio_group.scl_pin)
    {
        /* 初始化 GPIO */
        int ret = oled_gpio_init();
        if(ret < 0) {
            printk(KERN_ERR "%s: Failed to allocate GPIO\n", SPI_OLED_NAME);
            return ret;
        }
    }
        
    return 0;
}

/**
 * @Description: close 函数
 * @param {inode} *inode: 
 * @param {file} *file: 
 * @return {*}
 */
static int oled_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: Closing spi_oled device, free GPIO!\n", SPI_OLED_NAME);
    /* 取消 GPIO 占用 */
    oled_gpio_free();
    return 0;
}

/**
 * @Description: 返回 oled 信息，可使用 cat /dev/spi_oled
 * @param {file} *file: 文件结构体指针
 * @param {char __user} *user_buffer: 用户空间的内存地址
 * @param {size_t} count: 要读的长度
 * @param {loff_t} *offset: 读的位置相对于文件开头的偏移
 * @return {*}
 */
static ssize_t oled_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    char *usage_info;
    size_t len;

    // 动态生成使用方法
    usage_info = kasprintf(GFP_KERNEL,
        "Device Information:\n"
        "  Resolution: %d * %d\n"
        "  Buffer size: %ld Byte\n",
        FRAME_WIDTH, FRAME_HEIGHT, buffer_size);

    if (!usage_info) {
        return -ENOMEM;  // 内存分配失败
    }

    len = strlen(usage_info);

    // 如果偏移量已经超过字符串长度，返回 0 表示 EOF
    if (*offset >= len) {
        kfree(usage_info);
        return 0;
    }

    // 计算本次读取的字节数
    if (count > len - *offset) {
        count = len - *offset;
    }

    // 将数据复制到用户空间
    if (copy_to_user(user_buffer, usage_info + *offset, count)) {
        kfree(usage_info);
        return -EFAULT;  // 复制失败，返回错误
    }

    *offset += count;  // 更新偏移量
    kfree(usage_info); // 释放动态分配的内存
    return count;      // 返回实际读取的字节数
}

/**
 * @Description: 用户空间 write 函数，可以直接写入到帧缓冲
 * @param {file} *file: 文件结构体指针
 * @param {char __user} *user_buffer: 用户空间的内存地址
 * @param {size_t} count: 要写的长度
 * @param {loff_t} *offset: 写的位置相对于文件开头的偏移
 * @return {*}
 */
static ssize_t oled_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) { 
    bool ret;
    /* 检查是否已经配置 GPIO */
    ret = oled_gpio_check();
    if (ret == false)
    {
        printk(KERN_ERR "%s: Please init GPIO first!\n", SPI_OLED_NAME);
        return -ENODEV;
    }

    // 检查写入大小是否合法
    if (count > buffer_size) {
        printk(KERN_ERR "%s: ERROR! Write size %zu exceeds buffer size %ld\n", SPI_OLED_NAME, count, buffer_size);
        return -EINVAL;
    }

    // 将数据从用户空间复制到帧缓冲
    // copy_from_user 的 size 参数传入的是块大小（单次写入的大小）
    if (copy_from_user(spi_oled_dev.frame_buffer + *ppos, buf, count)) {
        printk(KERN_ERR "%s: Failed to copy data from user space\n", SPI_OLED_NAME);
        return -EFAULT;
    }

    // 刷新屏幕
    refresh_oled();

    // 更新文件位置
    *ppos = 0;

    printk(KERN_INFO "%s: Write succeeded: count=%zu, pos=%lld\n", SPI_OLED_NAME, count, *ppos);
    return count;
}


/**
 * @Description: 用户空间 ioctl 函数
 * @param {file} *file: 文件结构体指针
 * @param {unsigned int} cmd: 用户程序对设备的控制命令
 * @param {unsigned long} arg: 传输的数据
 * @return {*}
 */
static long oled_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    bool ret;
    /* 检查是否已经配置 GPIO */
    if (cmd != IOCTL_OLED_SET_GPIO)
    {
        ret = oled_gpio_check();
        if (ret == false)
        {
            printk(KERN_ERR "%s: Please init GPIO first!\n", SPI_OLED_NAME);
            return -ENODEV;
        }
    }

    switch (cmd) {
        /* 设置 GPIO 引脚号 */ 
        case IOCTL_OLED_SET_GPIO: {
            struct oled_gpio_stuct gpio_group_temp;
            int ret;

            /* 检查是否已经配置 GPIO */
            ret = oled_gpio_check();
            if (ret == true)
            {
                printk(KERN_INFO "%s: GPIO has been configured. Nothing to do.\n", SPI_OLED_NAME);
                return 0;
            }
            printk(KERN_INFO "%s: Tyring to init GPIO!\n", SPI_OLED_NAME);
            ret = copy_from_user(&gpio_group_temp, (void __user *)arg, sizeof(struct oled_gpio_stuct));
            if (ret < 0)
                return ret;  // 复制失败，返回错误

            /* 保存到设备结构体 */
            memcpy(&spi_oled_dev.gpio_group, &gpio_group_temp, sizeof(struct oled_gpio_stuct));
            
            /* 申请并初始化 GPIO */
            ret = oled_gpio_init();
            if(ret < 0) {
                printk(KERN_ERR "%s: Failed to allocate GPIO\n", SPI_OLED_NAME);
                return ret;
            }

            /* 初始化 OLED */
            oled_start_init();

            /* 初始完成，进行清空 */
            clear_oled();
            
            printk(KERN_INFO "%s: The OLED init success!\n", SPI_OLED_NAME);
            break;
        }
        /* 开启 OLED */ 
        case IOCTL_OLED_OPEN:
            // printk(KERN_INFO "%s: OLED turned on\n", SPI_OLED_NAME);
            open_oled();
            break;
        /* 关闭 OLED */ 
        case IOCTL_OLED_CLOSE:
            // printk(KERN_INFO "%s: OLED turned off\n", SPI_OLED_NAME);
            close_oled();
            break;
        /* 刷新 OLED */ 
        case IOCTL_OLED_REFRESH: 
            // printk(KERN_INFO "%s: OLED refreshed\n", SPI_OLED_NAME);
            refresh_oled();
            break;
        /* 清空 OLED */ 
        case IOCTL_OLED_CLEAR: 
            // printk(KERN_INFO "%s: OLED refreshed\n", SPI_OLED_NAME);
            clear_oled();
            break;
        default:
            printk(KERN_ERR "%s: Unknown command!\n", SPI_OLED_NAME);
            return -ENOTTY;
    }

    return 0;
}

/**
 * @Description: 用于映射 oled 帧缓冲
 * @param {file} *filp: 指向文件对象的指针
 * @param {struct vm_area_struct} *vma: 指向虚拟内存区域（VMA）的指针
 * @return {*}
 */
static int oled_mmap(struct file *filp, struct vm_area_struct *vma) {
    // 这里会返回页对齐后的地址大小
    unsigned long size = vma->vm_end - vma->vm_start; 

    // 检查用户空间申请的大小是否合法
    if (size < FRAME_BUFFER_SIZE) {
        printk(KERN_INFO "%s: ERROR! Size: %ld is less than %ld!\n", SPI_OLED_NAME, size, buffer_size);
        return -EINVAL;// 用户空间申请的大小不足
    }

    // 设置 vma 的权限
    // VM_IO 用于映射 I/O 设备，明确告诉内核这是一个设备内存区域，避免内核对其进行不必要的优化或管理
    // VM_DONTEXPAND 禁止该内存区域通过 mremap 系统调用进行扩展，确保该区域的大小固定，防止用户空间意外扩展
    // VM_DONTDUMP 禁止该内存区域被包含在核心转储（core dump）中，避免泄露敏感数据或占用不必要的磁盘空间
    vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

    // printk(KERN_INFO "vma: start=%lx, end=%lx, flags=%lx\n", vma->vm_start, vma->vm_end, vma->vm_flags);
    // printk(KERN_INFO "frame_buffer: %p, size: %zu\n", spi_oled_dev.frame_buffer, buffer_size);

    // 将帧缓冲区映射到用户空间
    if (remap_vmalloc_range(vma, spi_oled_dev.frame_buffer, 0)) {
        printk(KERN_INFO "%s: remap_vmalloc_range error!\n", SPI_OLED_NAME);
        return -EAGAIN;
    }

    return 0;
}

/**
 * @description : file 操作集
 * @param : 无
 * @return : 无
 */
static const struct file_operations spi_oled_fops = {
    .owner = THIS_MODULE,
    .open = oled_open,
    .release = oled_release,
    .read = oled_read,
    .write = oled_write,
    .unlocked_ioctl = oled_ioctl,
    .mmap = oled_mmap,
};

/***************************** 字符设备初始化 ******************************/
/**
 * @description : 驱动模块加载函数
 * @param : 无
 * @return : 无
 */
static int __init oled_driver_init(void) {
    int ret;

    /************ 帧缓冲 ************/
    // 内核在分配虚拟内存区域（VMA）时，可能会出于性能或管理方便的考虑，将映射大小调整为页面大小的整数倍
    // 即使只请求了 1024 字节，内核也会分配一个完整的页面（4096 字节）
    // remap_vmalloc_range 要求 vma 的大小不能超过 vmalloc 分配的内存大小

    /* 计算映射需要的大小 */
    size_t num_pages = FRAME_BUFFER_SIZE / PAGE_SIZE;
    if (FRAME_BUFFER_SIZE % PAGE_SIZE != 0) {
        num_pages += 1; // 如果不是整数倍，增加一页
    }
    buffer_size = num_pages * PAGE_SIZE;

    /* 分配帧缓冲区 */
    printk(KERN_INFO "%s: buffer_size: %zu\n", SPI_OLED_NAME, buffer_size);
    spi_oled_dev.frame_buffer = vmalloc_user(buffer_size);
    if (!spi_oled_dev.frame_buffer) {
        printk(KERN_ERR "%s: Failed to allocate frame buffer\n", SPI_OLED_NAME);
        goto free_gpio;
    }
    memset(spi_oled_dev.frame_buffer, 0, buffer_size);
    
    /************ 注册字符设备驱动 ************/
    /* 1、创建设备号 */
    if (spi_oled_dev.major) { /* 定义了设备号 */
        spi_oled_dev.devid = MKDEV(spi_oled_dev.major, 0);
        ret = register_chrdev_region(spi_oled_dev.devid, SPI_OLED_CNT, SPI_OLED_NAME);
        if(ret < 0) {
            printk(KERN_ERR "%s: Cannot register char driver [ret=%d]\n",SPI_OLED_NAME, SPI_OLED_CNT);
            goto free_buffer;
        }
    }
    else { /* 没有定义设备号 */
        ret = alloc_chrdev_region(&spi_oled_dev.devid, 0, SPI_OLED_CNT, SPI_OLED_NAME); /* 申请设备号 */
        if(ret < 0) {
            printk(KERN_ERR "%s: Couldn't alloc_chrdev_region,ret=%d\r\n", SPI_OLED_NAME, ret);
            goto free_buffer;
        }
        spi_oled_dev.major = MAJOR(spi_oled_dev.devid); /* 获取分配号的主设备号 */
        spi_oled_dev.minor = MINOR(spi_oled_dev.devid); /* 获取分配号的次设备号 */
    }
    printk(KERN_INFO "%s: spi_oled_dev major=%d, minor=%d\r\n", SPI_OLED_NAME, spi_oled_dev.major, spi_oled_dev.minor);

    /* 2、初始化 cdev */
    spi_oled_dev.cdev.owner = THIS_MODULE;
    cdev_init(&spi_oled_dev.cdev, &spi_oled_fops);

    /* 3、添加一个 cdev */
    cdev_add(&spi_oled_dev.cdev, spi_oled_dev.devid, SPI_OLED_CNT);
    if(ret < 0)
        goto del_unregister;

    /* 4、创建类 */
    spi_oled_dev.class = class_create(THIS_MODULE, SPI_OLED_NAME);
    if (IS_ERR(spi_oled_dev.class)) {
        goto del_cdev;
    }

    /* 5、创建设备 */
    spi_oled_dev.device = device_create(spi_oled_dev.class, NULL, spi_oled_dev.devid, NULL, SPI_OLED_NAME);
    if (IS_ERR(spi_oled_dev.device)) {
        goto destroy_class;
    }

    printk(KERN_INFO "%s: spi_oled driver is loaded!\n", SPI_OLED_NAME);
    return 0;

destroy_class:
    class_destroy(spi_oled_dev.class);
del_cdev:
    cdev_del(&spi_oled_dev.cdev);
del_unregister:
    unregister_chrdev_region(spi_oled_dev.devid, SPI_OLED_CNT);
free_buffer:
    vfree(spi_oled_dev.frame_buffer);
free_gpio:
    oled_gpio_free();
    return -EIO;
}

/**
 * @description : 驱动模块卸载函数
 * @param : 无
 * @return : 无
 */
static void __exit oled_driver_exit(void) {

    /* 注销字符设备驱动 */
    device_destroy(spi_oled_dev.class, spi_oled_dev.devid);     /* 注销设备 */
    class_destroy(spi_oled_dev.class);                          /* 注销类 */
    cdev_del(&spi_oled_dev.cdev);                               /* 删除 cdev */
    unregister_chrdev_region(spi_oled_dev.devid, SPI_OLED_CNT); /* 注销设备号 */
    vfree(spi_oled_dev.frame_buffer);                           /* 释放帧缓冲 */
    oled_gpio_free();                                           /* 释放 GPIO */

    printk(KERN_INFO "%s: spi_oled driver is removed!\n", SPI_OLED_NAME);
}

module_init(oled_driver_init);
module_exit(oled_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Li-Rongfu");
MODULE_INFO(intree, "Y");
MODULE_DESCRIPTION("OLED char-device driver with mmap");