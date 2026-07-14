AMIGA_PREFIX ?= /opt/amiga
CROSS := $(AMIGA_PREFIX)/bin/m68k-amigaos-
CC := $(CROSS)gcc
AS := $(AMIGA_PREFIX)/bin/vasmm68k_mot

CPPFLAGS := -Iinclude
CFLAGS := -O2 -Wall -Wextra -m68000 -mcrt=nix13 -fno-builtin
DEVICE_OBJS := build/device.o build/hardware_stub.o build/callback.o build/romtag.o

.PHONY: all clean
all: build/sana2skeleton.device build/test_sana_open

build:
	mkdir -p build

build/device.o: src/device.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

build/hardware_stub.o: src/hardware_stub.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

build/callback.o: src/callback.asm | build
	$(AS) -Fhunk -quiet -o $@ $<

build/romtag.o: src/romtag.asm | build
	$(AS) -Fhunk -quiet -o $@ $<

build/sana2skeleton.device: $(DEVICE_OBJS)
	$(CC) -nostdlib -m68000 -o $@ $(DEVICE_OBJS)

build/test_sana_open: tools/test_sana_open.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -rf build
