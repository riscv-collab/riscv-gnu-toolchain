#as: -march=rv64gc -mabi=lp64d
#ld: -m elf64lriscv -e0
#PROG: objcopy
#objcopy: -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .reloc --target=pei-riscv64-little
#objdump: -h -f
#name: Check if efi app format is recognized

.*:     file format pei-riscv64-little
architecture: riscv:rv64, flags 0x00000132:
EXEC_P, HAS_SYMS, HAS_LOCALS, D_PAGED
start address 0x0000000000000000

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  0 \.text         00000010  0[^ ]+  0[^ ]+  0[^ ]+  2\*\*2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
