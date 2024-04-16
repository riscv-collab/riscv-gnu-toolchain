#as: -EB -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: alu32-pseudoc.s
#name: eBPF ALU32 instructions, big-endian, pseudo-c syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	04 20 00 00 00 00 02 9a 	w2\+=0x29a
   8:	04 30 00 00 ff ff fd 66 	w3\+=0xfffffd66
  10:	04 40 00 00 7e ad be ef 	w4\+=0x7eadbeef
  18:	0c 56 00 00 00 00 00 00 	w5\+=w6
  20:	14 20 00 00 00 00 02 9a 	w2-=0x29a
  28:	14 30 00 00 ff ff fd 66 	w3-=0xfffffd66
  30:	14 40 00 00 7e ad be ef 	w4-=0x7eadbeef
  38:	1c 56 00 00 00 00 00 00 	w5-=w6
  40:	24 20 00 00 00 00 02 9a 	w2\*=0x29a
  48:	24 30 00 00 ff ff fd 66 	w3\*=0xfffffd66
  50:	24 40 00 00 7e ad be ef 	w4\*=0x7eadbeef
  58:	2c 56 00 00 00 00 00 00 	w5\*=w6
  60:	34 20 00 00 00 00 02 9a 	w2/=0x29a
  68:	34 30 00 00 ff ff fd 66 	w3/=0xfffffd66
  70:	34 40 00 00 7e ad be ef 	w4/=0x7eadbeef
  78:	3c 56 00 00 00 00 00 00 	w5/=w6
  80:	44 20 00 00 00 00 02 9a 	w2|=0x29a
  88:	44 30 00 00 ff ff fd 66 	w3|=0xfffffd66
  90:	44 40 00 00 7e ad be ef 	w4|=0x7eadbeef
  98:	4c 56 00 00 00 00 00 00 	w5|=w6
  a0:	54 20 00 00 00 00 02 9a 	w2&=0x29a
  a8:	54 30 00 00 ff ff fd 66 	w3&=0xfffffd66
  b0:	54 40 00 00 7e ad be ef 	w4&=0x7eadbeef
  b8:	5c 56 00 00 00 00 00 00 	w5&=w6
  c0:	64 20 00 00 00 00 02 9a 	w2<<=0x29a
  c8:	64 30 00 00 ff ff fd 66 	w3<<=0xfffffd66
  d0:	64 40 00 00 7e ad be ef 	w4<<=0x7eadbeef
  d8:	6c 56 00 00 00 00 00 00 	w5<<=w6
  e0:	74 20 00 00 00 00 02 9a 	w2>>=0x29a
  e8:	74 30 00 00 ff ff fd 66 	w3>>=0xfffffd66
  f0:	74 40 00 00 7e ad be ef 	w4>>=0x7eadbeef
  f8:	7c 56 00 00 00 00 00 00 	w5>>=w6
 100:	94 20 00 00 00 00 02 9a 	w2%=0x29a
 108:	94 30 00 00 ff ff fd 66 	w3%=0xfffffd66
 110:	94 40 00 00 7e ad be ef 	w4%=0x7eadbeef
 118:	9c 56 00 00 00 00 00 00 	w5%=w6
 120:	a4 20 00 00 00 00 02 9a 	w2\^=0x29a
 128:	a4 30 00 00 ff ff fd 66 	w3\^=0xfffffd66
 130:	a4 40 00 00 7e ad be ef 	w4\^=0x7eadbeef
 138:	ac 56 00 00 00 00 00 00 	w5\^=w6
 140:	b4 20 00 00 00 00 02 9a 	w2=0x29a
 148:	b4 30 00 00 ff ff fd 66 	w3=0xfffffd66
 150:	b4 40 00 00 7e ad be ef 	w4=0x7eadbeef
 158:	bc 56 00 00 00 00 00 00 	w5=w6
 160:	c4 20 00 00 00 00 02 9a 	w2 s>>=0x29a
 168:	c4 30 00 00 ff ff fd 66 	w3 s>>=0xfffffd66
 170:	c4 40 00 00 7e ad be ef 	w4 s>>=0x7eadbeef
 178:	cc 56 00 00 00 00 00 00 	w5 s>>=w6
 180:	84 20 00 00 00 00 00 00 	w2=-w2
 188:	bc 12 00 08 00 00 00 00 	w1 = \(s8\) w2
 190:	bc 12 00 10 00 00 00 00 	w1 = \(s16\) w2
 198:	bc 12 00 20 00 00 00 00 	w1 = \(s32\) w2
