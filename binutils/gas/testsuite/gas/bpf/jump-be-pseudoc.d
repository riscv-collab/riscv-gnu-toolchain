#as: -EB -mdialect=pseudoc
#source: jump-pseudoc.s
#objdump: -dr -M dec,pseudoc
#name: eBPF JUMP instructions, big endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	05 00 00 03 00 00 00 00 	goto 3
   8:	0f 11 00 00 00 00 00 00 	r1\+=r1
  10:	15 30 00 01 00 00 00 03 	if r3==3 goto 1
  18:	1d 34 00 00 00 00 00 00 	if r3==r4 goto 0
  20:	35 30 ff fd 00 00 00 03 	if r3>=3 goto -3
  28:	3d 34 ff fc 00 00 00 00 	if r3>=r4 goto -4
  30:	a5 30 00 01 00 00 00 03 	if r3<3 goto 1
  38:	ad 34 00 00 00 00 00 00 	if r3<r4 goto 0
  40:	b5 30 00 01 00 00 00 03 	if r3<=3 goto 1
  48:	bd 34 00 00 00 00 00 00 	if r3<=r4 goto 0
  50:	45 30 00 01 00 00 00 03 	if r3&3 goto 1
  58:	4d 34 00 00 00 00 00 00 	if r3&r4 goto 0
  60:	55 30 00 01 00 00 00 03 	if r3!=3 goto 1
  68:	5d 34 00 00 00 00 00 00 	if r3!=r4 goto 0
  70:	65 30 00 01 00 00 00 03 	if r3s>3 goto 1
  78:	6d 34 00 00 00 00 00 00 	if r3s>r4 goto 0
  80:	75 30 00 01 00 00 00 03 	if r3s>=3 goto 1
  88:	7d 34 00 00 00 00 00 00 	if r3s>=r4 goto 0
  90:	c5 30 00 01 00 00 00 03 	if r3s<3 goto 1
  98:	cd 34 00 00 00 00 00 00 	if r3s<r4 goto 0
  a0:	d5 30 00 01 00 00 00 03 	if r3s<=3 goto 1
  a8:	dd 34 00 00 00 00 00 00 	if r3s<=r4 goto 0
  b0:	06 00 00 00 00 00 00 01 	gotol 1
  b8:	06 00 00 00 00 00 00 00 	gotol 0
