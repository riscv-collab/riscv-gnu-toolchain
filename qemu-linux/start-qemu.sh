#!/bin/sh

../bin/qemu-system-riscv64 \
    -M virt \
    -nographic \
    -bios ./fw_dynamic.bin \
    -kernel ./Image \
    -drive file=./rootfs.ext2,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -append "rootwait root=/dev/vda"
