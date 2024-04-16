#as:
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

0+ <.*>:
   0:	1a000004 	pcalau12i   	\$a0, 0
			0: R_LARCH_TLS_DESC_PC_HI20	var
   4:	02c00084 	addi.d      	\$a0, \$a0, 0
			4: R_LARCH_TLS_DESC_PC_LO12	var
   8:	28c00081 	ld.d        	\$ra, \$a0, 0
			8: R_LARCH_TLS_DESC_LD	var
   c:	4c000021 	jirl        	\$ra, \$ra, 0
			c: R_LARCH_TLS_DESC_CALL	var
  10:	1a000004 	pcalau12i   	\$a0, 0
			10: R_LARCH_TLS_DESC_PC_HI20	var
			10: R_LARCH_RELAX	\*ABS\*
  14:	02c00084 	addi.d      	\$a0, \$a0, 0
			14: R_LARCH_TLS_DESC_PC_LO12	var
			14: R_LARCH_RELAX	\*ABS\*
  18:	28c00081 	ld.d        	\$ra, \$a0, 0
			18: R_LARCH_TLS_DESC_LD	var
			18: R_LARCH_RELAX	\*ABS\*
  1c:	4c000021 	jirl        	\$ra, \$ra, 0
			1c: R_LARCH_TLS_DESC_CALL	var
			1c: R_LARCH_RELAX	\*ABS\*
