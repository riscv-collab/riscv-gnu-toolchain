#as:
#objdump: -dr
#skip: loongarch32-*-*

.*:[    ]+file format .*


Disassembly of section .text:

[ 	]*0000000000000000 <.text>:
[ 	]+0:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+0: R_LARCH_TLS_DESC_PC_HI20[ 	]+var
[ 	]+4:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+4: R_LARCH_TLS_DESC_PC_LO12[ 	]+var
[ 	]+8:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+8: R_LARCH_TLS_DESC64_PC_LO20[ 	]+var
[ 	]+c:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+c: R_LARCH_TLS_DESC64_PC_HI12[ 	]+var
[ 	]+10:[ 	]+00109484[ 	]+add.d[ 	]+\$a0, \$a0, \$a1
[ 	]+14:[ 	]+28c00081[ 	]+ld.d[ 	]+\$ra, \$a0, 0
[ 	]+14: R_LARCH_TLS_DESC_LD[ 	]+var
[ 	]+18:[ 	]+4c000021[ 	]+jirl[ 	]+\$ra, \$ra, 0
[ 	]+18: R_LARCH_TLS_DESC_CALL[ 	]+var
[ 	]+1c:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+1c: R_LARCH_TLS_DESC_PC_HI20[ 	]+var
[ 	]+20:[ 	]+02c00001[ 	]+li.d[ 	]+\$ra, 0
[ 	]+20: R_LARCH_TLS_DESC_PC_LO12[ 	]+var
[ 	]+24:[ 	]+16000001[ 	]+lu32i.d[ 	]+\$ra, 0
[ 	]+24: R_LARCH_TLS_DESC64_PC_LO20[ 	]+var
[ 	]+28:[ 	]+03000021[ 	]+lu52i.d[ 	]+\$ra, \$ra, 0
[ 	]+28: R_LARCH_TLS_DESC64_PC_HI12[ 	]+var
[ 	]+2c:[ 	]+00108484[ 	]+add.d[ 	]+\$a0, \$a0, \$ra
[ 	]+30:[ 	]+28c00081[ 	]+ld.d[ 	]+\$ra, \$a0, 0
[ 	]+30: R_LARCH_TLS_DESC_LD[ 	]+var
[ 	]+34:[ 	]+4c000021[ 	]+jirl[ 	]+\$ra, \$ra, 0
[ 	]+34: R_LARCH_TLS_DESC_CALL[ 	]+var
