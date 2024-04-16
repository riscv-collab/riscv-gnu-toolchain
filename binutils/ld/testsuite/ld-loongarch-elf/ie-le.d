#as:
#ld: -z norelro -e0
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

[0-9a-f]+ <fn1>:
   +[0-9a-f]+:	14000004 	lu12i.w     	\$a0, .*
   +[0-9a-f]+:	03800084 	ori         	\$a0, \$a0, .*
   +[0-9a-f]+:	1a000084 	pcalau12i   	\$a0, .*
   +[0-9a-f]+:	02c44005 	li.d        	\$a1, .*
   +[0-9a-f]+:	16000005 	lu32i.d     	\$a1, .*
   +[0-9a-f]+:	030000a5 	lu52i.d     	\$a1, \$a1, .*
   +[0-9a-f]+:	380c1484 	ldx.d       	\$a0, \$a0, \$a1
