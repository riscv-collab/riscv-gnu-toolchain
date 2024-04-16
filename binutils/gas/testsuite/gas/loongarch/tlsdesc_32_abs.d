#as: -mla-global-with-abs
#objdump: -dr
#skip: loongarch64-*-*

.*:     file format .*


Disassembly of section .text:

0+ <.*>:
   0:	14000004 	lu12i.w     	\$a0, 0
			0: R_LARCH_TLS_DESC_HI20	var
   4:	03800084 	ori         	\$a0, \$a0, 0x0
			4: R_LARCH_TLS_DESC_LO12	var
   8:	28800081 	ld.w        	\$ra, \$a0, 0
			8: R_LARCH_TLS_DESC_LD	var
   c:	4c000021 	jirl        	\$ra, \$ra, 0
			c: R_LARCH_TLS_DESC_CALL	var
  10:	14000004 	lu12i.w     	\$a0, 0
			10: R_LARCH_TLS_DESC_HI20	var
  14:	03800084 	ori         	\$a0, \$a0, 0x0
			14: R_LARCH_TLS_DESC_LO12	var
  18:	28800081 	ld.w        	\$ra, \$a0, 0
			18: R_LARCH_TLS_DESC_LD	var
  1c:	4c000021 	jirl        	\$ra, \$ra, 0
			1c: R_LARCH_TLS_DESC_CALL	var
