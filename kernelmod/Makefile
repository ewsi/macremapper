


##
## To build for an Ubuntu host machine...
##
## Make sure you have installed the Linux headers:
## $ sudo apt-get install linux-headers-`uname -r`
##
## Uncomment the following:
#KERNEL_SRC ?= /usr/src/linux-headers-`uname -r`



##
## To cross-compile for the RDK XB3 ARM core
##
## Uncomment the following:
#RDK_BUILD            ?= /home/jdennis/build/arrisxb3/build-xb3
#KERNEL_SRC           ?= $(RDK_BUILD)/tmp/sysroots/arrisxb3arm/usr/src/kernel
#export CROSS_COMPILE ?= $(RDK_BUILD)/tmp/sysroots/x86_64-linux/usr/bin/arm1176jzstb-rdk-linux-uclibceabi/armeb-rdk-linux-uclibceabi-
#export ARCH          ?= arm

##
## To cross-compile for the RDK XB3 ATOM core
##
## Uncomment the following:
#RDK_BUILD            ?= /home/jdennis/build/arrisxb3/build-xb3
#KERNEL_SRC           ?= $(RDK_BUILD)/tmp/sysroots/arrisxb3atom/usr/src/kernel
#export CROSS_COMPILE ?= $(RDK_BUILD)/tmp/sysroots/x86_64-linux/usr/bin/core2-32-rdk-linux/i586-rdk-linux-


ifeq ($(KERNEL_SRC),)
$(error KERNEL_SRC not defined!)
endif


MODULE_PATH := $(shell MODULE_PATH)



MODULE_NAME := macremapper

$(MODULE_NAME)-objs := $(patsubst $(MODULE_PATH)/%.c,%.o,$(patsubst %.mod.c,,$(wildcard $(MODULE_PATH)/*.c)))
obj-m := $(MODULE_NAME).o


.PHONY: all clean modinfo

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_PATH) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_PATH) clean

modinfo:
	modinfo $(MODULE_NAME).ko

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_PATH) $@

