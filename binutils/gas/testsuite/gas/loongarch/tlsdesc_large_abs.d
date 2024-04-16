#as: -mla-global-with-abs
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

0+ <.*>:
   0:	14000004 	lu12i.w     	\$a0, 0
			0: R_LARCH_TLS_DESC_HI20	var
   4:	03800084 	ori         	\$a0, \$a0, 0x0
			4: R_LARCH_TLS_DESC_LO12	var
   8:	16000004 	lu32i.d     	\$a0, 0
			8: R_LARCH_TLS_DESC64_LO20	var
   c:	03000084 	lu52i.d     	\$a0, \$a0, 0
			c: R_LARCH_TLS_DESC64_HI12	var
  10:	28c00081 	ld.d        	\$ra, \$a0, 0
			10: R_LARCH_TLS_DESC_LD	var
  14:	4c000021 	jirl        	\$ra, \$ra, 0
			14: R_LARCH_TLS_DESC_CALL	var
  18:	14000004 	lu12i.w     	\$a0, 0
			18: R_LARCH_TLS_DESC_HI20	var
  1c:	03800084 	ori         	\$a0, \$a0, 0x0
			1c: R_LARCH_TLS_DESC_LO12	var
  20:	16000004 	lu32i.d     	\$a0, 0
			20: R_LARCH_TLS_DESC64_LO20	var
  24:	03000084 	lu52i.d     	\$a0, \$a0, 0
			24: R_LARCH_TLS_DESC64_HI12	var
  28:	28c00081 	ld.d        	\$ra, \$a0, 0
			28: R_LARCH_TLS_DESC_LD	var
  2c:	4c000021 	jirl        	\$ra, \$ra, 0
			2c: R_LARCH_TLS_DESC_CALL	var
