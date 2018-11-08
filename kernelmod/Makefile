


##
## To build for an Ubuntu host machine...
##
## Make sure you have installed the Linux headers:
## $ sudo apt-get install linux-headers-`uname -r`
##
## Uncomment the following:
KERNEL_PATH ?= /usr/src/linux-headers-`uname -r`



##
## To cross-compile for the RDK XB3 ARM core
##
## Uncomment the following:
#RDK_BUILD            ?= /home/jdennis/build/arrisxb3/build-xb3
#LINUX_ROOT           ?= $(RDK_BUILD)/tmp/sysroots/arrisxb3arm/usr/src/kernel
#export CROSS_COMPILE ?= $(RDK_BUILD)/tmp/sysroots/x86_64-linux/usr/bin/arm1176jzstb-rdk-linux-uclibceabi/armeb-rdk-linux-uclibceabi-
#export ARCH          ?= arm

##
## To cross-compile for the RDK XB3 ATOM core
##
## Uncomment the following:
#RDK_BUILD            ?= /home/jdennis/build/arrisxb3/build-xb3
#LINUX_ROOT           ?= $(RDK_BUILD)/tmp/sysroots/arrisxb3atom/usr/src/kernel
#export CROSS_COMPILE ?= $(RDK_BUILD)/tmp/sysroots/x86_64-linux/usr/bin/core2-32-rdk-linux/i586-rdk-linux-


ifeq ($(KERNEL_PATH),)
$(error KERNEL_PATH not defined!)
endif






MODULE_NAME := macremapper

$(MODULE_NAME)-objs := $(patsubst $(PWD)/%.c,%.o,$(patsubst %.mod.c,,$(wildcard $(PWD)/*.c)))
obj-m := $(MODULE_NAME).o


.PHONY: all clean modinfo serve

all:
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD) modules
	$(MAKE) modinfo

clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD) clean

modinfo:
	modinfo $(MODULE_NAME).ko

modules_install:
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD) modules_install

