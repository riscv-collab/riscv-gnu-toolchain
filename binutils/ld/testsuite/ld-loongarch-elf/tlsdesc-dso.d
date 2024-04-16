#as:
#ld: -shared -z norelro --hash-style=both
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

0+418 <fun_gl1>:
 418:	180214c4 	pcaddi      	\$a0, 4262
 41c:	1a000084 	pcalau12i   	\$a0, 4
 420:	28db0084 	ld.d        	\$a0, \$a0, 1728
 424:	180212a4 	pcaddi      	\$a0, 4245
 428:	18021304 	pcaddi      	\$a0, 4248
 42c:	28c00081 	ld.d        	\$ra, \$a0, 0
 430:	4c000021 	jirl        	\$ra, \$ra, 0
 434:	1a000084 	pcalau12i   	\$a0, 4
 438:	28d9c084 	ld.d        	\$a0, \$a0, 1648
 43c:	03400000 	nop.*
 440:	03400000 	nop.*
 444:	1a000084 	pcalau12i   	\$a0, 4
 448:	28d9c084 	ld.d        	\$a0, \$a0, 1648
 44c:	18021264 	pcaddi      	\$a0, 4243
 450:	18021244 	pcaddi      	\$a0, 4242
 454:	28c00081 	ld.d        	\$ra, \$a0, 0
 458:	4c000021 	jirl        	\$ra, \$ra, 0
 45c:	1a000084 	pcalau12i   	\$a0, 4
 460:	28daa084 	ld.d        	\$a0, \$a0, 1704

0+464 <fun_lo>:
 464:	1a000084 	pcalau12i   	\$a0, 4
 468:	28d86084 	ld.d        	\$a0, \$a0, 1560
 46c:	18020ce4 	pcaddi      	\$a0, 4199
 470:	18020e04 	pcaddi      	\$a0, 4208
 474:	28c00081 	ld.d        	\$ra, \$a0, 0
 478:	4c000021 	jirl        	\$ra, \$ra, 0
 47c:	18020d24 	pcaddi      	\$a0, 4201
 480:	1a000084 	pcalau12i   	\$a0, 4
 484:	28d90084 	ld.d        	\$a0, \$a0, 1600
 488:	03400000 	nop.*
 48c:	03400000 	nop.*
 490:	1a000084 	pcalau12i   	\$a0, 4
 494:	28d90084 	ld.d        	\$a0, \$a0, 1600
 498:	18020d84 	pcaddi      	\$a0, 4204
 49c:	28c00081 	ld.d        	\$ra, \$a0, 0
 4a0:	4c000021 	jirl        	\$ra, \$ra, 0
 4a4:	18020d24 	pcaddi      	\$a0, 4201
 4a8:	1a000084 	pcalau12i   	\$a0, 4
 4ac:	28d96084 	ld.d        	\$a0, \$a0, 1624

0+4b0 <fun_external>:
 4b0:	18020d84 	pcaddi      	\$a0, 4204
 4b4:	28c00081 	ld.d        	\$ra, \$a0, 0
 4b8:	4c000021 	jirl        	\$ra, \$ra, 0
