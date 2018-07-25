


##
## To build for an Ubuntu host machine...
##
## Make sure you have installed the Linux headers:
## $ sudo apt-get install linux-headers-`uname -r`
##
## Uncomment the following:
#LINUX_ROOT := /usr/src/linux-headers-`uname -r`



##
## To cross-compile for the RDK XB3 ARM core
##
## Uncomment the following:
RDK_BUILD  := /home/jdennis/build/arrisxb3/build-xb3
LINUX_ROOT := $(RDK_BUILD)/tmp/sysroots/arrisxb3arm/usr/src/kernel
export ARCH := arm
export CROSS_COMPILE := $(RDK_BUILD)/tmp/sysroots/x86_64-linux/usr/bin/arm1176jzstb-rdk-linux-uclibceabi/armeb-rdk-linux-uclibceabi-

##
## To cross-compile for the RDK XB3 ATOM core
##
## Uncomment the following:
#RDK_BUILD  := /home/jdennis/build/arrisxb3/build-xb3
#LINUX_ROOT := $(RDK_BUILD)/tmp/sysroots/arrisxb3atom/usr/src/kernel
#export CROSS_COMPILE := $(RDK_BUILD)/tmp/sysroots/x86_64-linux/usr/bin/core2-32-rdk-linux/i586-rdk-linux-


ifeq ($(LINUX_ROOT),)
$(error LINUX_ROOT not defined!)
endif






MODULE_NAME := macremapper

$(MODULE_NAME)-objs := $(patsubst $(PWD)/%.c,%.o,$(patsubst %.mod.c,,$(wildcard $(PWD)/*.c)))
obj-m := $(MODULE_NAME).o


.PHONY: all clean modinfo serve

all:
	$(MAKE) -C $(LINUX_ROOT) M=$(PWD) modules
	$(MAKE) modinfo

clean:
	$(MAKE) -C $(LINUX_ROOT) M=$(PWD) clean

modinfo:
	modinfo $(MODULE_NAME).ko

serve: all
	 scp $(MODULE_NAME).ko 10.32.32.92:/build/tftproot/
