#name: pcrel-reloc
#source: pcrel-reloc.s
#as:
#ld: -Ttext 0x0 --defsym foo=0x1337
#objdump: -dr

.*:     file format elf64-kvx


Disassembly of section .text:

0000000000000000 <_start>:
   0:	00 00 d0 8f                                     	ret
   4:	c0 cd 04 f0 04 00 00 80 00 00 00 00             	pcrel \$r1 = 4919 \(0x1337\);;
