#as: -EB -mdialect=pseudoc
#source: alu-pseudoc.s
#objdump: -dr -M hex,pseudoc
#name: eBPF ALU64 instructions, big endian, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	07 20 00 00 00 00 02 9a 	r2\+=0x29a
   8:	07 30 00 00 ff ff fd 66 	r3\+=0xfffffd66
  10:	07 40 00 00 7e ad be ef 	r4\+=0x7eadbeef
  18:	0f 56 00 00 00 00 00 00 	r5\+=r6
  20:	17 20 00 00 00 00 02 9a 	r2-=0x29a
  28:	17 30 00 00 ff ff fd 66 	r3-=0xfffffd66
  30:	17 40 00 00 7e ad be ef 	r4-=0x7eadbeef
  38:	1f 56 00 00 00 00 00 00 	r5-=r6
  40:	27 20 00 00 00 00 02 9a 	r2\*=0x29a
  48:	27 30 00 00 ff ff fd 66 	r3\*=0xfffffd66
  50:	27 40 00 00 7e ad be ef 	r4\*=0x7eadbeef
  58:	2f 56 00 00 00 00 00 00 	r5\*=r6
  60:	37 20 00 00 00 00 02 9a 	r2/=0x29a
  68:	37 30 00 00 ff ff fd 66 	r3/=0xfffffd66
  70:	37 40 00 00 7e ad be ef 	r4/=0x7eadbeef
  78:	3f 56 00 00 00 00 00 00 	r5/=r6
  80:	47 20 00 00 00 00 02 9a 	r2|=0x29a
  88:	47 30 00 00 ff ff fd 66 	r3|=0xfffffd66
  90:	47 40 00 00 7e ad be ef 	r4|=0x7eadbeef
  98:	4f 56 00 00 00 00 00 00 	r5|=r6
  a0:	57 20 00 00 00 00 02 9a 	r2&=0x29a
  a8:	57 30 00 00 ff ff fd 66 	r3&=0xfffffd66
  b0:	57 40 00 00 7e ad be ef 	r4&=0x7eadbeef
  b8:	5f 56 00 00 00 00 00 00 	r5&=r6
  c0:	67 20 00 00 00 00 02 9a 	r2<<=0x29a
  c8:	67 30 00 00 ff ff fd 66 	r3<<=0xfffffd66
  d0:	67 40 00 00 7e ad be ef 	r4<<=0x7eadbeef
  d8:	6f 56 00 00 00 00 00 00 	r5<<=r6
  e0:	77 20 00 00 00 00 02 9a 	r2>>=0x29a
  e8:	77 30 00 00 ff ff fd 66 	r3>>=0xfffffd66
  f0:	77 40 00 00 7e ad be ef 	r4>>=0x7eadbeef
  f8:	7f 56 00 00 00 00 00 00 	r5>>=r6
 100:	97 20 00 00 00 00 02 9a 	r2%=0x29a
 108:	97 30 00 00 ff ff fd 66 	r3%=0xfffffd66
 110:	97 40 00 00 7e ad be ef 	r4%=0x7eadbeef
 118:	9f 56 00 00 00 00 00 00 	r5%=r6
 120:	a7 20 00 00 00 00 02 9a 	r2\^=0x29a
 128:	a7 30 00 00 ff ff fd 66 	r3\^=0xfffffd66
 130:	a7 40 00 00 7e ad be ef 	r4\^=0x7eadbeef
 138:	af 56 00 00 00 00 00 00 	r5\^=r6
 140:	b7 20 00 00 00 00 02 9a 	r2=0x29a
 148:	b7 30 00 00 ff ff fd 66 	r3=0xfffffd66
 150:	b7 40 00 00 7e ad be ef 	r4=0x7eadbeef
 158:	bf 56 00 00 00 00 00 00 	r5=r6
 160:	c7 20 00 00 00 00 02 9a 	r2 s>>=0x29a
 168:	c7 30 00 00 ff ff fd 66 	r3 s>>=0xfffffd66
 170:	c7 40 00 00 7e ad be ef 	r4 s>>=0x7eadbeef
 178:	cf 56 00 00 00 00 00 00 	r5 s>>=r6
 180:	87 20 00 00 00 00 00 00 	r2=-r2
 188:	d4 90 00 00 00 00 00 10 	r9=le16 r9
 190:	d4 80 00 00 00 00 00 20 	r8=le32 r8
 198:	d4 70 00 00 00 00 00 40 	r7=le64 r7
 1a0:	dc 60 00 00 00 00 00 10 	r6=be16 r6
 1a8:	dc 50 00 00 00 00 00 20 	r5=be32 r5
 1b0:	dc 40 00 00 00 00 00 40 	r4=be64 r4
 1b8:	bf 12 00 08 00 00 00 00 	r1 = \(s8\) r2
 1c0:	bf 12 00 10 00 00 00 00 	r1 = \(s16\) r2
 1c8:	bf 12 00 20 00 00 00 00 	r1 = \(s32\) r2
 1d0:	d7 10 00 00 00 00 00 10 	r1 = bswap16 r1
 1d8:	d7 20 00 00 00 00 00 20 	r2 = bswap32 r2
 1e0:	d7 30 00 00 00 00 00 40 	r3 = bswap64 r3
 1e8:	b7 20 00 00 00 00 02 9a 	r2=0x29a
