#as: -march=kv3-1
#objdump: -d
#source: fat-bundles.s
.*\/fat-bundles.o:     file format elf64-kvx


Disassembly of section .text:

0000000000000000 <.text>:
   0:	82 34 04 e1 80 84 00 e1                         	addd \$r1 = \$r2, 1234 \(0x4d2\)
   8:	80 84 00 e1 01 00 00 80 46 a6 2f 8f             	addd \$r0 = \$r0, 123456789010 \(0x1cbe991a12\)
  14:	00 00 00 88 46 a6 2f 97 00 00 00 10             	addd \$r0 = \$r0, 123456789010 \(0x1cbe991a12\);;

  20:	00 00 d8 8f                                     	igoto \$r0
  24:	00 00 0c 84                                     	xmt44d \$a0a1a2a3 = \$a0a1a2a3
  28:	80 34 00 e1 40 0d 00 e1                         	addd \$r0 = \$r0, 1234 \(0x4d2\)
  30:	c2 70 05 d8 00 00 00 b0                         	addd \$r0 = \$r0, 12345678901 \(0x2dfdc1c35\)
  38:	01 00 00 80                                     	fmuld \$r1 = \$r2, \$r3
  3c:	07 f7 b7 08                                     	lwz \$r0 = 0 \(0x0\)\[\$r0\];;

