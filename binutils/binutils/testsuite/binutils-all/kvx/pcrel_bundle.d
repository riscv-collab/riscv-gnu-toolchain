#name: pcrel_bundle
#source: pcrel_bundle.s
#PROG: objcopy
#as:
#objdump: -dr
#...

Disassembly of section .text:

0000000000000000 <foo>:
   0:	00 0e 00 f0 00 00 00 80 00 00 00 00             	pcrel \$r0 = 56 \(0x38\);;

   c:	0d 00 00 98                                     	call 40 <bar>
  10:	00 0b 00 f0 00 00 00 80 00 00 00 00             	pcrel \$r0 = 44 \(0x2c\);;

  1c:	09 00 00 98                                     	call 40 <bar>
  20:	00 07 00 f0 00 00 00 b8 00 00 00 80             	pcrel \$r0 = 28 \(0x1c\)
  2c:	00 00 00 00                                     	ld \$r0 = 0 \(0x0\)\[\$r0\];;

  30:	00 f0 03 7f                                     	nop;;

  34:	00 f0 03 7f                                     	nop;;


0000000000000038 <.table>:
  38:	00 f0 03 7f                                     	nop;;

  3c:	00 f0 03 7f                                     	nop;;


0000000000000040 <bar>:
  40:	00 f0 03 7f                                     	nop;;

