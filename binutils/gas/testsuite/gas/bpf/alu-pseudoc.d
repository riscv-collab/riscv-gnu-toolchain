#as: -EL -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: alu-pseudoc.s
#name: eBPF ALU instructions, pseudo-c syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	07 02 00 00 9a 02 00 00 	r2\+=0x29a
   8:	07 03 00 00 66 fd ff ff 	r3\+=0xfffffd66
  10:	07 04 00 00 ef be ad 7e 	r4\+=0x7eadbeef
  18:	0f 65 00 00 00 00 00 00 	r5\+=r6
  20:	17 02 00 00 9a 02 00 00 	r2-=0x29a
  28:	17 03 00 00 66 fd ff ff 	r3-=0xfffffd66
  30:	17 04 00 00 ef be ad 7e 	r4-=0x7eadbeef
  38:	1f 65 00 00 00 00 00 00 	r5-=r6
  40:	27 02 00 00 9a 02 00 00 	r2\*=0x29a
  48:	27 03 00 00 66 fd ff ff 	r3\*=0xfffffd66
  50:	27 04 00 00 ef be ad 7e 	r4\*=0x7eadbeef
  58:	2f 65 00 00 00 00 00 00 	r5\*=r6
  60:	37 02 00 00 9a 02 00 00 	r2/=0x29a
  68:	37 03 00 00 66 fd ff ff 	r3/=0xfffffd66
  70:	37 04 00 00 ef be ad 7e 	r4/=0x7eadbeef
  78:	3f 65 00 00 00 00 00 00 	r5/=r6
  80:	47 02 00 00 9a 02 00 00 	r2|=0x29a
  88:	47 03 00 00 66 fd ff ff 	r3|=0xfffffd66
  90:	47 04 00 00 ef be ad 7e 	r4|=0x7eadbeef
  98:	4f 65 00 00 00 00 00 00 	r5|=r6
  a0:	57 02 00 00 9a 02 00 00 	r2&=0x29a
  a8:	57 03 00 00 66 fd ff ff 	r3&=0xfffffd66
  b0:	57 04 00 00 ef be ad 7e 	r4&=0x7eadbeef
  b8:	5f 65 00 00 00 00 00 00 	r5&=r6
  c0:	67 02 00 00 9a 02 00 00 	r2<<=0x29a
  c8:	67 03 00 00 66 fd ff ff 	r3<<=0xfffffd66
  d0:	67 04 00 00 ef be ad 7e 	r4<<=0x7eadbeef
  d8:	6f 65 00 00 00 00 00 00 	r5<<=r6
  e0:	77 02 00 00 9a 02 00 00 	r2>>=0x29a
  e8:	77 03 00 00 66 fd ff ff 	r3>>=0xfffffd66
  f0:	77 04 00 00 ef be ad 7e 	r4>>=0x7eadbeef
  f8:	7f 65 00 00 00 00 00 00 	r5>>=r6
 100:	97 02 00 00 9a 02 00 00 	r2%=0x29a
 108:	97 03 00 00 66 fd ff ff 	r3%=0xfffffd66
 110:	97 04 00 00 ef be ad 7e 	r4%=0x7eadbeef
 118:	9f 65 00 00 00 00 00 00 	r5%=r6
 120:	a7 02 00 00 9a 02 00 00 	r2\^=0x29a
 128:	a7 03 00 00 66 fd ff ff 	r3\^=0xfffffd66
 130:	a7 04 00 00 ef be ad 7e 	r4\^=0x7eadbeef
 138:	af 65 00 00 00 00 00 00 	r5\^=r6
 140:	b7 02 00 00 9a 02 00 00 	r2=0x29a
 148:	b7 03 00 00 66 fd ff ff 	r3=0xfffffd66
 150:	b7 04 00 00 ef be ad 7e 	r4=0x7eadbeef
 158:	bf 65 00 00 00 00 00 00 	r5=r6
 160:	c7 02 00 00 9a 02 00 00 	r2 s>>=0x29a
 168:	c7 03 00 00 66 fd ff ff 	r3 s>>=0xfffffd66
 170:	c7 04 00 00 ef be ad 7e 	r4 s>>=0x7eadbeef
 178:	cf 65 00 00 00 00 00 00 	r5 s>>=r6
 180:	87 02 00 00 00 00 00 00 	r2=-r2
 188:	d4 09 00 00 10 00 00 00 	r9=le16 r9
 190:	d4 08 00 00 20 00 00 00 	r8=le32 r8
 198:	d4 07 00 00 40 00 00 00 	r7=le64 r7
 1a0:	dc 06 00 00 10 00 00 00 	r6=be16 r6
 1a8:	dc 05 00 00 20 00 00 00 	r5=be32 r5
 1b0:	dc 04 00 00 40 00 00 00 	r4=be64 r4
 1b8:	bf 21 08 00 00 00 00 00 	r1 = \(s8\) r2
 1c0:	bf 21 10 00 00 00 00 00 	r1 = \(s16\) r2
 1c8:	bf 21 20 00 00 00 00 00 	r1 = \(s32\) r2
 1d0:	d7 01 00 00 10 00 00 00 	r1 = bswap16 r1
 1d8:	d7 02 00 00 20 00 00 00 	r2 = bswap32 r2
 1e0:	d7 03 00 00 40 00 00 00 	r3 = bswap64 r3
 1e8:	b7 02 00 00 9a 02 00 00 	r2=0x29a