#as: -EL -mdialect=pseudoc
#objdump: -dr -M hex
#name: BPF assembler comments - pseudoc

.*: +file format .*bpf.*

Disassembly of section .text:

[0-9a-f]+ <.*>:
   0:	07 02 00 00 9a 02 00 00 	add %r2,0x29a
   8:	07 03 00 00 66 fd ff ff 	add %r3,0xfffffd66
  10:	07 04 00 00 ef be ad 7e 	add %r4,0x7eadbeef
