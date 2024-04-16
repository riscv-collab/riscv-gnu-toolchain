#as: -EL -mdialect=normal
#objdump: -dr -M dec
#source: jump-relax-jump.s
#name: Relaxation of conditional branch instructions

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.*>:
   0:	1d 21 00 80 00 00 00 00 	jeq %r1,%r2,-32768
   8:	ad 21 ff 7f 00 00 00 00 	jlt %r1,%r2,32767
  10:	bd 21 fd ff 00 00 00 00 	jle %r1,%r2,-3
  18:	1d 21 01 00 00 00 00 00 	jeq %r1,%r2,1
  20:	05 00 01 00 00 00 00 00 	ja 1
  28:	06 00 00 00 01 80 00 00 	jal 32769
  30:	2d 21 01 00 00 00 00 00 	jgt %r1,%r2,1
  38:	05 00 01 00 00 00 00 00 	ja 1
  40:	06 00 00 00 01 80 00 00 	jal 32769
