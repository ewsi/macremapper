MODULE_NAME := macremapper
#LINUX_ROOT := /home/jdennis/build/arrisxb3/build-xb3/tmp/sysroots/arrisxb3atom/usr/src/kernel
LINUX_ROOT := /home/jdennis/build/arrisxb3/build-xb3/tmp/sysroots/arrisxb3arm/usr/src/kernel
#LINUX_ROOT := /usr/src/linux-headers-`uname -r`

export ARCH := arm
export CROSS_COMPILE := /home/jdennis/build/arrisxb3/build-xb3/tmp/sysroots/x86_64-linux/usr/bin/arm1176jzstb-rdk-linux-uclibceabi/armeb-rdk-linux-uclibceabi-

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
