#as: -mla-global-with-abs -mla-local-with-abs
#objdump: -dr
#skip: loongarch32-*-*

.*:     file format .*


Disassembly of section .text:

0+ <.L1>:
   0:	14000004 	lu12i.w     	\$a0, 0
			0: R_LARCH_MARK_LA	\*ABS\*
			0: R_LARCH_ABS_HI20	.L1
   4:	03800084 	ori         	\$a0, \$a0, 0x0
			4: R_LARCH_ABS_LO12	.L1
   8:	16000004 	lu32i.d     	\$a0, 0
			8: R_LARCH_ABS64_LO20	.L1
   c:	03000084 	lu52i.d     	\$a0, \$a0, 0
			c: R_LARCH_ABS64_HI12	.L1
  10:	14000004 	lu12i.w     	\$a0, 0
			10: R_LARCH_MARK_LA	\*ABS\*
			10: R_LARCH_ABS_HI20	.L1
  14:	03800084 	ori         	\$a0, \$a0, 0x0
			14: R_LARCH_ABS_LO12	.L1
  18:	16000004 	lu32i.d     	\$a0, 0
			18: R_LARCH_ABS64_LO20	.L1
  1c:	03000084 	lu52i.d     	\$a0, \$a0, 0
			1c: R_LARCH_ABS64_HI12	.L1
  20:	1a000004 	pcalau12i   	\$a0, 0
			20: R_LARCH_PCALA_HI20	.L1
  24:	02c00084 	addi.d      	\$a0, \$a0, 0
			24: R_LARCH_PCALA_LO12	.L1
  28:	14000004 	lu12i.w     	\$a0, 0
			28: R_LARCH_GOT_HI20	.L1
  2c:	03800084 	ori         	\$a0, \$a0, 0x0
			2c: R_LARCH_GOT_LO12	.L1
  30:	16000004 	lu32i.d     	\$a0, 0
			30: R_LARCH_GOT64_LO20	.L1
  34:	03000084 	lu52i.d     	\$a0, \$a0, 0
			34: R_LARCH_GOT64_HI12	.L1
  38:	28c00084 	ld.d        	\$a0, \$a0, 0
  3c:	14000004 	lu12i.w     	\$a0, 0
			3c: R_LARCH_TLS_LE_HI20	TLS1
  40:	03800084 	ori         	\$a0, \$a0, 0x0
			40: R_LARCH_TLS_LE_LO12	TLS1
  44:	14000004 	lu12i.w     	\$a0, 0
			44: R_LARCH_TLS_IE_HI20	TLS1
  48:	03800084 	ori         	\$a0, \$a0, 0x0
			48: R_LARCH_TLS_IE_LO12	TLS1
  4c:	16000004 	lu32i.d     	\$a0, 0
			4c: R_LARCH_TLS_IE64_LO20	TLS1
  50:	03000084 	lu52i.d     	\$a0, \$a0, 0
			50: R_LARCH_TLS_IE64_HI12	TLS1
  54:	28c00084 	ld.d        	\$a0, \$a0, 0
  58:	14000004 	lu12i.w     	\$a0, 0
			58: R_LARCH_TLS_LD_HI20	TLS1
  5c:	03800084 	ori         	\$a0, \$a0, 0x0
			5c: R_LARCH_GOT_LO12	TLS1
  60:	16000004 	lu32i.d     	\$a0, 0
			60: R_LARCH_GOT64_LO20	TLS1
  64:	03000084 	lu52i.d     	\$a0, \$a0, 0
			64: R_LARCH_GOT64_HI12	TLS1
  68:	14000004 	lu12i.w     	\$a0, 0
			68: R_LARCH_TLS_GD_HI20	TLS1
  6c:	03800084 	ori         	\$a0, \$a0, 0x0
			6c: R_LARCH_GOT_LO12	TLS1
  70:	16000004 	lu32i.d     	\$a0, 0
			70: R_LARCH_GOT64_LO20	TLS1
  74:	03000084 	lu52i.d     	\$a0, \$a0, 0
			74: R_LARCH_GOT64_HI12	TLS1
