
CROSS_COMPILE := /home/jdennis/build/arrisxb3/build-xb3/tmp/sysroots/x86_64-linux/usr/bin/arm1176jzstb-rdk-linux-uclibceabi/armeb-rdk-linux-uclibceabi-

CC            := $(CROSS_COMPILE)gcc
STRIP         := $(CROSS_COMPILE)strip

CFLAGS        := -O3 -c -Wall
LINKFLAGS     := 

BUILDDIR      := build


OBJECTS       := $(patsubst %.c,$(BUILDDIR)/%.o,$(wildcard *.c))


OUTPUT_NAME := $(BUILDDIR)/mrmctl

.PHONY: all clean serve


all:
	$(MAKE) $(BUILDDIR)
	$(MAKE) $(OUTPUT_NAME)

clean:
	rm -Rf $(BUILDDIR)

serve: all
	scp $(OUTPUT_NAME) 10.32.32.92:/build/tftproot/


$(BUILDDIR):
	mkdir -p $@

$(OUTPUT_NAME): $(OBJECTS)
	$(CC) $(LINKFLAGS) -o $@ $(OBJECTS)
	$(STRIP) $@


$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<


