#as: -EB -mdialect=normal
#objdump: -dr -M dec
#source: jump32.s
#name: eBPF JUMP32 instructions, normal syntax, big-endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	05 00 00 03 00 00 00 00 	ja 3
   8:	0f 11 00 00 00 00 00 00 	add %r1,%r1
  10:	16 30 00 01 00 00 00 03 	jeq32 %r3,3,1
  18:	1e 34 00 00 00 00 00 00 	jeq32 %r3,%r4,0
  20:	36 30 ff fd 00 00 00 03 	jge32 %r3,3,-3
  28:	3e 34 ff fc 00 00 00 00 	jge32 %r3,%r4,-4
  30:	a6 30 00 01 00 00 00 03 	jlt32 %r3,3,1
  38:	ae 34 00 00 00 00 00 00 	jlt32 %r3,%r4,0
  40:	b6 30 00 01 00 00 00 03 	jle32 %r3,3,1
  48:	be 34 00 00 00 00 00 00 	jle32 %r3,%r4,0
  50:	46 30 00 01 00 00 00 03 	jset32 %r3,3,1
  58:	4e 34 00 00 00 00 00 00 	jset32 %r3,%r4,0
  60:	56 30 00 01 00 00 00 03 	jne32 %r3,3,1
  68:	5e 34 00 00 00 00 00 00 	jne32 %r3,%r4,0
  70:	66 30 00 01 00 00 00 03 	jsgt32 %r3,3,1
  78:	6e 34 00 00 00 00 00 00 	jsgt32 %r3,%r4,0
  80:	76 30 00 01 00 00 00 03 	jsge32 %r3,3,1
  88:	7e 34 00 00 00 00 00 00 	jsge32 %r3,%r4,0
  90:	c6 30 00 01 00 00 00 03 	jslt32 %r3,3,1
  98:	ce 34 00 00 00 00 00 00 	jslt32 %r3,%r4,0
  a0:	d6 30 00 01 00 00 00 03 	jsle32 %r3,3,1
  a8:	de 34 00 00 00 00 00 00 	jsle32 %r3,%r4,0
