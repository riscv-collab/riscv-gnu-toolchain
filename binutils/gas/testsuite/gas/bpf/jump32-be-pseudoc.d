#as: -EB -mdialect=pseudoc
#objdump: -dr -M dec,pseudoc
#source: jump32-pseudoc.s
#name: eBPF JUMP32 instructions, pseudoc syntax, big-endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	05 00 00 03 00 00 00 00 	goto 3
   8:	0f 11 00 00 00 00 00 00 	r1\+=r1
  10:	16 30 00 01 00 00 00 03 	if w3==3 goto 1
  18:	1e 34 00 00 00 00 00 00 	if w3==w4 goto 0
  20:	36 30 ff fd 00 00 00 03 	if w3>=3 goto -3
  28:	3e 34 ff fc 00 00 00 00 	if w3>=w4 goto -4
  30:	a6 30 00 01 00 00 00 03 	if w3<3 goto 1
  38:	ae 34 00 00 00 00 00 00 	if w3<w4 goto 0
  40:	b6 30 00 01 00 00 00 03 	if w3<=3 goto 1
  48:	be 34 00 00 00 00 00 00 	if w3<=w4 goto 0
  50:	46 30 00 01 00 00 00 03 	if w3&3 goto 1
  58:	4e 34 00 00 00 00 00 00 	if w3&w4 goto 0
  60:	56 30 00 01 00 00 00 03 	if w3!=3 goto 1
  68:	5e 34 00 00 00 00 00 00 	if w3!=w4 goto 0
  70:	66 30 00 01 00 00 00 03 	if w3s>3 goto 1
  78:	6e 34 00 00 00 00 00 00 	if w3s>w4 goto 0
  80:	76 30 00 01 00 00 00 03 	if w3s>=3 goto 1
  88:	7e 34 00 00 00 00 00 00 	if w3s>=w4 goto 0
  90:	c6 30 00 01 00 00 00 03 	if w3s<3 goto 1
  98:	ce 34 00 00 00 00 00 00 	if w3s<w4 goto 0
  a0:	d6 30 00 01 00 00 00 03 	if w3s<=3 goto 1
  a8:	de 34 00 00 00 00 00 00 	if w3s<=w4 goto 0
