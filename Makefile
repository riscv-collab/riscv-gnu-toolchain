# Based on xv6-riscv Makefile; see LICENSE.

TOOLPREFIX=riscv64-unknown-elf-
QEMU=qemu-system-riscv64

CC=$(TOOLPREFIX)gcc
AS=$(TOOLPREFIX)as
LD=$(TOOLPREFIX)ld
OBJCOPY=$(TOOLPREFIX)objcopy
OBJDUMP=$(TOOLPREFIX)objdump

CORE_OBJS = \
	start.o \
	halt.o \
	string.o \
	trap.o \
	intr.o \
	serial.o \
	console.o \
	vga.o \
	mp1.o

CFLAGS = -Wall -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-pie -no-pie -march=rv64g -mabi=lp64d
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -fno-asynchronous-unwind-tables
CFLAGS += -I. -DDEBUG -DTRACE

all: demo.elf

demo.elf: $(CORE_OBJS)
	$(LD) $(LDFLAGS) -T kernel.ld -o $@ $^ demo.o gold.o memory.o

clean:
	rm -rf console.o halt.o intr.o serial.o start.o string.o trap.o vga.o mp1.o *.elf

run-demo:
	$(QEMU) -machine virt -bios none -kernel demo.elf -m 128M -serial mon:stdio -device bochs-display
