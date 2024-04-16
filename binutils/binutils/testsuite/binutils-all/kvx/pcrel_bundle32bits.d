#name: pcrel_bundle
#source: pcrel_bundle.s
#PROG: objcopy
#as: -m32
#objdump: -dr
#...

Disassembly of section .text:

00000000 <foo>:
   0:	00 0b 00 f0 00 00 00 00                         	pcrel \$r0 = 44 \(0x2c\);;

   8:	0b 00 00 98                                     	call 34 <bar>
   c:	00 09 00 f0 00 00 00 00                         	pcrel \$r0 = 36 \(0x24\);;

  14:	08 00 00 98                                     	call 34 <bar>
  18:	00 06 00 f0 00 00 00 b8                         	pcrel \$r0 = 24 \(0x18\)
  20:	00 00 00 00                                     	ld \$r0 = 0 \(0x0\)\[\$r0\];;

  24:	00 f0 03 7f                                     	nop;;

  28:	00 f0 03 7f                                     	nop;;


0000002c <.table>:
  2c:	00 f0 03 7f                                     	nop;;

  30:	00 f0 03 7f                                     	nop;;


00000034 <bar>:
  34:	00 f0 03 7f                                     	nop;;

