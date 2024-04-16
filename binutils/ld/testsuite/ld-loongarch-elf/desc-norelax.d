#as:
#ld: -z norelro -shared --section-start=.got=0x1ff000 --hash-style=both
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

0+1c0 <.*>:
 1c0:	1a003fe4 	pcalau12i   	\$a0, 511
 1c4:	02c02084 	addi.d      	\$a0, \$a0, 8
 1c8:	28c00081 	ld.d        	\$ra, \$a0, 0
 1cc:	4c000021 	jirl        	\$ra, \$ra, 0
 1d0:	0010888c 	add.d       	\$t0, \$a0, \$tp
