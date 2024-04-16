#as: -EB -mdialect=pseudoc
#source: spacing-pseudoc.s
#objdump: -dr -M hex,pseudoc
#name: spacing, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	b7 04 00 00 ef be ad de 	r4=0xdeadbeef
   8:	18 04 00 00 ef be ad de 	r4=0xdeadbeef ll
  10:	00 00 00 00 00 00 00 00 
  18:	05 00 01 00 00 00 00 00 	goto 0x1
  20:	05 00 01 00 00 00 00 00 	goto 0x1
  28:	05 00 01 00 00 00 00 00 	goto 0x1
  30:	16 03 01 00 03 00 00 00 	if w3==0x3 goto 0x1
  38:	16 03 01 00 03 00 00 00 	if w3==0x3 goto 0x1
