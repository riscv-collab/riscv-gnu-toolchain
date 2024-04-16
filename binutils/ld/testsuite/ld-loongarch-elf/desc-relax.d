#as:
#ld: -z norelro -shared --hash-style=both
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

0+188 <.*>:
 188:	18020844 	pcaddi      	\$a0, 4162
 18c:	28c00081 	ld.d        	\$ra, \$a0, 0
 190:	4c000021 	jirl        	\$ra, \$ra, 0
 194:	0010888c 	add.d       	\$t0, \$a0, \$tp
