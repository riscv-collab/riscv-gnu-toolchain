# Based on xv6-riscv Makefile; see LICENSE.

TOOLPREFIX=riscv64-unknown-elf-
QEMU=qemu-system-riscv64

CC=$(TOOLPREFIX)gcc
AS=$(TOOLPREFIX)as
LD=$(TOOLPREFIX)ld
OBJCOPY=$(TOOLPREFIX)objcopy
OBJDUMP=$(TOOLPREFIX)objdump

CFLAGS = -Wall -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-pie -no-pie -march=rv64g -mabi=lp64d
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.

kernel.elf: start.o main.o cons.o com.o halt.o trek.o
	$(LD) -T kernel.ld -o $@ $^

clean:
	rm -rf start.o main.o cons.o com.o halt.o kernel.elf

# Run with
#   qemu-system-riscv64 -machine virt -bios none -kernel kernel.elf -m 128M -nographic
# or
#   qemu-system-riscv64 -machine virt -bios none -kernel kernel.elf -m 128M -display gtk
