#as: -EB -mdialect=pseudoc
#source: lddw-pseudoc.s
#objdump: -dr -M hex,pseudoc
#name: eBPF LDDW, big-endian, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	18 30 00 00 00 00 00 01 	r3=0x1 ll
   8:	00 00 00 00 00 00 00 00 
  10:	18 40 00 00 de ad be ef 	r4=0xdeadbeef ll
  18:	00 00 00 00 00 00 00 00 
  20:	18 50 00 00 55 66 77 88 	r5=0x1122334455667788 ll
  28:	00 00 00 00 11 22 33 44 
  30:	18 60 00 00 ff ff ff fe 	r6=0xfffffffffffffffe ll
  38:	00 00 00 00 ff ff ff ff 
