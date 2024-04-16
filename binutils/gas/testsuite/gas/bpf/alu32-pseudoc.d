#as: -EL -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: alu32-pseudoc.s
#name: eBPF ALU32 instructions, pseudo-c syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	04 02 00 00 9a 02 00 00 	w2\+=0x29a
   8:	04 03 00 00 66 fd ff ff 	w3\+=0xfffffd66
  10:	04 04 00 00 ef be ad 7e 	w4\+=0x7eadbeef
  18:	0c 65 00 00 00 00 00 00 	w5\+=w6
  20:	14 02 00 00 9a 02 00 00 	w2-=0x29a
  28:	14 03 00 00 66 fd ff ff 	w3-=0xfffffd66
  30:	14 04 00 00 ef be ad 7e 	w4-=0x7eadbeef
  38:	1c 65 00 00 00 00 00 00 	w5-=w6
  40:	24 02 00 00 9a 02 00 00 	w2\*=0x29a
  48:	24 03 00 00 66 fd ff ff 	w3\*=0xfffffd66
  50:	24 04 00 00 ef be ad 7e 	w4\*=0x7eadbeef
  58:	2c 65 00 00 00 00 00 00 	w5\*=w6
  60:	34 02 00 00 9a 02 00 00 	w2/=0x29a
  68:	34 03 00 00 66 fd ff ff 	w3/=0xfffffd66
  70:	34 04 00 00 ef be ad 7e 	w4/=0x7eadbeef
  78:	3c 65 00 00 00 00 00 00 	w5/=w6
  80:	44 02 00 00 9a 02 00 00 	w2|=0x29a
  88:	44 03 00 00 66 fd ff ff 	w3|=0xfffffd66
  90:	44 04 00 00 ef be ad 7e 	w4|=0x7eadbeef
  98:	4c 65 00 00 00 00 00 00 	w5|=w6
  a0:	54 02 00 00 9a 02 00 00 	w2&=0x29a
  a8:	54 03 00 00 66 fd ff ff 	w3&=0xfffffd66
  b0:	54 04 00 00 ef be ad 7e 	w4&=0x7eadbeef
  b8:	5c 65 00 00 00 00 00 00 	w5&=w6
  c0:	64 02 00 00 9a 02 00 00 	w2<<=0x29a
  c8:	64 03 00 00 66 fd ff ff 	w3<<=0xfffffd66
  d0:	64 04 00 00 ef be ad 7e 	w4<<=0x7eadbeef
  d8:	6c 65 00 00 00 00 00 00 	w5<<=w6
  e0:	74 02 00 00 9a 02 00 00 	w2>>=0x29a
  e8:	74 03 00 00 66 fd ff ff 	w3>>=0xfffffd66
  f0:	74 04 00 00 ef be ad 7e 	w4>>=0x7eadbeef
  f8:	7c 65 00 00 00 00 00 00 	w5>>=w6
 100:	94 02 00 00 9a 02 00 00 	w2%=0x29a
 108:	94 03 00 00 66 fd ff ff 	w3%=0xfffffd66
 110:	94 04 00 00 ef be ad 7e 	w4%=0x7eadbeef
 118:	9c 65 00 00 00 00 00 00 	w5%=w6
 120:	a4 02 00 00 9a 02 00 00 	w2\^=0x29a
 128:	a4 03 00 00 66 fd ff ff 	w3\^=0xfffffd66
 130:	a4 04 00 00 ef be ad 7e 	w4\^=0x7eadbeef
 138:	ac 65 00 00 00 00 00 00 	w5\^=w6
 140:	b4 02 00 00 9a 02 00 00 	w2=0x29a
 148:	b4 03 00 00 66 fd ff ff 	w3=0xfffffd66
 150:	b4 04 00 00 ef be ad 7e 	w4=0x7eadbeef
 158:	bc 65 00 00 00 00 00 00 	w5=w6
 160:	c4 02 00 00 9a 02 00 00 	w2 s>>=0x29a
 168:	c4 03 00 00 66 fd ff ff 	w3 s>>=0xfffffd66
 170:	c4 04 00 00 ef be ad 7e 	w4 s>>=0x7eadbeef
 178:	cc 65 00 00 00 00 00 00 	w5 s>>=w6
 180:	84 02 00 00 00 00 00 00 	w2=-w2
 188:	bc 21 08 00 00 00 00 00 	w1 = \(s8\) w2
 190:	bc 21 10 00 00 00 00 00 	w1 = \(s16\) w2
 198:	bc 21 20 00 00 00 00 00 	w1 = \(s32\) w2
