#as: -EL -mdialect=pseudoc
#objdump: -dr -M dec,pseudoc
#source: jump-pseudoc.s
#name: eBPF JUMP instructions, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	05 00 03 00 00 00 00 00 	goto 3
   8:	0f 11 00 00 00 00 00 00 	r1\+=r1
  10:	15 03 01 00 03 00 00 00 	if r3==3 goto 1
  18:	1d 43 00 00 00 00 00 00 	if r3==r4 goto 0
  20:	35 03 fd ff 03 00 00 00 	if r3>=3 goto -3
  28:	3d 43 fc ff 00 00 00 00 	if r3>=r4 goto -4
  30:	a5 03 01 00 03 00 00 00 	if r3<3 goto 1
  38:	ad 43 00 00 00 00 00 00 	if r3<r4 goto 0
  40:	b5 03 01 00 03 00 00 00 	if r3<=3 goto 1
  48:	bd 43 00 00 00 00 00 00 	if r3<=r4 goto 0
  50:	45 03 01 00 03 00 00 00 	if r3&3 goto 1
  58:	4d 43 00 00 00 00 00 00 	if r3&r4 goto 0
  60:	55 03 01 00 03 00 00 00 	if r3!=3 goto 1
  68:	5d 43 00 00 00 00 00 00 	if r3!=r4 goto 0
  70:	65 03 01 00 03 00 00 00 	if r3s>3 goto 1
  78:	6d 43 00 00 00 00 00 00 	if r3s>r4 goto 0
  80:	75 03 01 00 03 00 00 00 	if r3s>=3 goto 1
  88:	7d 43 00 00 00 00 00 00 	if r3s>=r4 goto 0
  90:	c5 03 01 00 03 00 00 00 	if r3s<3 goto 1
  98:	cd 43 00 00 00 00 00 00 	if r3s<r4 goto 0
  a0:	d5 03 01 00 03 00 00 00 	if r3s<=3 goto 1
  a8:	dd 43 00 00 00 00 00 00 	if r3s<=r4 goto 0
  b0:	06 00 00 00 01 00 00 00 	gotol 1
  b8:	06 00 00 00 00 00 00 00 	gotol 0