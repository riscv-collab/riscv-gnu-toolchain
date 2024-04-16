#as: -EL -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: lddw-pseudoc.s
#name: eBPF LDDW, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	18 03 00 00 01 00 00 00 	r3=0x1 ll
   8:	00 00 00 00 00 00 00 00 
  10:	18 04 00 00 ef be ad de 	r4=0xdeadbeef ll
  18:	00 00 00 00 00 00 00 00 
  20:	18 05 00 00 88 77 66 55 	r5=0x1122334455667788 ll
  28:	00 00 00 00 44 33 22 11 
  30:	18 06 00 00 fe ff ff ff 	r6=0xfffffffffffffffe ll
  38:	00 00 00 00 ff ff ff ff 
