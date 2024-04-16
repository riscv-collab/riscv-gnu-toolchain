#as: --no-warn
#objdump: -dr
#skip: loongarch32-*-*

.*:[    ]+file format .*


Disassembly of section .text:

[ 	]*0000000000000000 <.Lla-relax-align>:
[ 	]+0:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+0: R_LARCH_PCALA_HI20[ 	]+L1
[ 	]+0: R_LARCH_RELAX[ 	]+\*ABS\*
[ 	]+4:[ 	]+02c00084[ 	]+addi.d[ 	]+\$a0, \$a0, 0
[ 	]+4: R_LARCH_PCALA_LO12[ 	]+L1
[ 	]+4: R_LARCH_RELAX[ 	]+\*ABS\*
[ 	]+8:[ 	]+03400000[ 	]+nop.*
[ 	]+8: R_LARCH_ALIGN[ 	]+.Lla-relax-align\+0x4
[ 	]+c:[ 	]+03400000[ 	]+nop.*
[ 	]+10:[ 	]+03400000[ 	]+nop.*
[ 	]+14:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+14: R_LARCH_PCALA_HI20[ 	]+L1
[ 	]+14: R_LARCH_RELAX[ 	]+\*ABS\*
[ 	]+18:[ 	]+02c00084[ 	]+addi.d[ 	]+\$a0, \$a0, 0
[ 	]+18: R_LARCH_PCALA_LO12[ 	]+L1
[ 	]+18: R_LARCH_RELAX[ 	]+\*ABS\*
[ 	]+1c:[ 	]+03400000[ 	]+nop.*
[ 	]+1c: R_LARCH_ALIGN[ 	]+.Lla-relax-align\+0x404
[ 	]+20:[ 	]+03400000[ 	]+nop.*
[ 	]+24:[ 	]+03400000[ 	]+nop.*
[ 	]+28:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+28: R_LARCH_PCALA_HI20[ 	]+L1
[ 	]+28: R_LARCH_RELAX[ 	]+\*ABS\*
[ 	]+2c:[ 	]+02c00084[ 	]+addi.d[ 	]+\$a0, \$a0, 0
[ 	]+2c: R_LARCH_PCALA_LO12[ 	]+L1
[ 	]+2c: R_LARCH_RELAX[ 	]+\*ABS\*
