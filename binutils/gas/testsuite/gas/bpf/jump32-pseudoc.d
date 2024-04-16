#as: -EL -mdialect=pseudoc
#objdump: -dr -M dec,pseudoc
#source: jump32-pseudoc.s
#name: eBPF JUMP32 instructions, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	05 00 03 00 00 00 00 00 	goto 3
   8:	0f 11 00 00 00 00 00 00 	r1\+=r1
  10:	16 03 01 00 03 00 00 00 	if w3==3 goto 1
  18:	1e 43 00 00 00 00 00 00 	if w3==w4 goto 0
  20:	36 03 fd ff 03 00 00 00 	if w3>=3 goto -3
  28:	3e 43 fc ff 00 00 00 00 	if w3>=w4 goto -4
  30:	a6 03 01 00 03 00 00 00 	if w3<3 goto 1
  38:	ae 43 00 00 00 00 00 00 	if w3<w4 goto 0
  40:	b6 03 01 00 03 00 00 00 	if w3<=3 goto 1
  48:	be 43 00 00 00 00 00 00 	if w3<=w4 goto 0
  50:	46 03 01 00 03 00 00 00 	if w3&3 goto 1
  58:	4e 43 00 00 00 00 00 00 	if w3&w4 goto 0
  60:	56 03 01 00 03 00 00 00 	if w3!=3 goto 1
  68:	5e 43 00 00 00 00 00 00 	if w3!=w4 goto 0
  70:	66 03 01 00 03 00 00 00 	if w3s>3 goto 1
  78:	6e 43 00 00 00 00 00 00 	if w3s>w4 goto 0
  80:	76 03 01 00 03 00 00 00 	if w3s>=3 goto 1
  88:	7e 43 00 00 00 00 00 00 	if w3s>=w4 goto 0
  90:	c6 03 01 00 03 00 00 00 	if w3s<3 goto 1
  98:	ce 43 00 00 00 00 00 00 	if w3s<w4 goto 0
  a0:	d6 03 01 00 03 00 00 00 	if w3s<=3 goto 1
  a8:	de 43 00 00 00 00 00 00 	if w3s<=w4 goto 0
