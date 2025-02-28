ifneq ($(KERNELRELEASE),) # 由内核构建系统调用, KERNELRELEASE 为当前内核的版本号
	obj-m:=spi_oled.o

else # 用户直接调用 make 时，KERNELRELEASE 为空
	KDIR :=/home/lrf/orangepi/linux-orangepi

PWD :=$(shell pwd)

all:
# 调用内核构建系统，会重新执行 Makefile，执行 KERNELRELEASE 不为空的情况
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif