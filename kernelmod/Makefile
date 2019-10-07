


##
## To build for an Ubuntu host machine...
##
## Make sure you have installed the Linux headers:
## $ sudo apt-get install linux-headers-`uname -r`
##
## Uncomment the following:
# KERNEL_SRC ?= /usr/src/linux-headers-`uname -r`



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

##
## To cross-compile for TP-Link OpenWRT build
##
## Uncomment the following:
#export ARCH          ?= mips
#WRT_DIR              ?= /home/jdennis/dcwifi_openwrt/openwrt
#KERNEL_SRC           ?= $(WRT_DIR)/build_dir/target-mips_24kc_musl/linux-ath79_generic/linux-4.14.93
#export CROSS_COMPILE ?= $(WRT_DIR)/staging_dir/toolchain-mips_24kc_gcc-7.4.0_musl/initial/bin/mips-openwrt-linux-musl-

##
## To cross-compile for Nighthawk OpenWRT build
##
## Uncomment the following:
#export ARCH          ?= arm
#WRT_DIR              ?= /build/ram/jdennis/openwrt.nighthawk
#KERNEL_SRC           ?= $(WRT_DIR)/build_dir/target-arm_cortex-a9_musl_eabi/linux-bcm53xx/linux-4.14.103
#export CROSS_COMPILE ?= $(WRT_DIR)/staging_dir/toolchain-arm_cortex-a9_gcc-7.4.0_musl_eabi/initial/bin/arm-openwrt-linux-muslgnueabi-




ifeq ($(KERNEL_SRC),)
$(error KERNEL_SRC not defined!)
endif


export MABSPATH ?= $(shell pwd)



MODULE_NAME := macremapper

$(MODULE_NAME)-objs := $(patsubst $(MABSPATH)/%.c,%.o,$(patsubst %.mod.c,,$(wildcard $(MABSPATH)/*.c)))
obj-m := $(MODULE_NAME).o


.PHONY: all clean modinfo

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(MABSPATH) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(MABSPATH) $@

modinfo:
	modinfo $(MODULE_NAME).ko

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(MABSPATH) $@

