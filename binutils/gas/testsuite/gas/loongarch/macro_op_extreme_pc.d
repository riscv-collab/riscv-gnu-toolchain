#as: -mla-global-with-pcrel
#objdump: -dr
#skip: loongarch32-*-*

.*:[    ]+file format .*


Disassembly of section .text:

[ 	]*0000000000000000 <.L1>:
[ 	]+0:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+0: R_LARCH_PCALA_HI20[ 	]+.L1
[ 	]+4:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+4: R_LARCH_PCALA_LO12[ 	]+.L1
[ 	]+8:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+8: R_LARCH_PCALA64_LO20[ 	]+.L1
[ 	]+c:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+c: R_LARCH_PCALA64_HI12[ 	]+.L1
[ 	]+10:[ 	]+00109484[ 	]+add.d[ 	]+\$a0, \$a0, \$a1
[ 	]+14:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+14: R_LARCH_PCALA_HI20[ 	]+.L1
[ 	]+18:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+18: R_LARCH_PCALA_LO12[ 	]+.L1
[ 	]+1c:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+1c: R_LARCH_PCALA64_LO20[ 	]+.L1
[ 	]+20:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+20: R_LARCH_PCALA64_HI12[ 	]+.L1
[ 	]+24:[ 	]+00109484[ 	]+add.d[ 	]+\$a0, \$a0, \$a1
[ 	]+28:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+28: R_LARCH_PCALA_HI20[ 	]+.L1
[ 	]+2c:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+2c: R_LARCH_PCALA_LO12[ 	]+.L1
[ 	]+30:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+30: R_LARCH_PCALA64_LO20[ 	]+.L1
[ 	]+34:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+34: R_LARCH_PCALA64_HI12[ 	]+.L1
[ 	]+38:[ 	]+00109484[ 	]+add.d[ 	]+\$a0, \$a0, \$a1
[ 	]+3c:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+3c: R_LARCH_GOT_PC_HI20[ 	]+.L1
[ 	]+40:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+40: R_LARCH_GOT_PC_LO12[ 	]+.L1
[ 	]+44:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+44: R_LARCH_GOT64_PC_LO20[ 	]+.L1
[ 	]+48:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+48: R_LARCH_GOT64_PC_HI12[ 	]+.L1
[ 	]+4c:[ 	]+380c1484[ 	]+ldx.d[ 	]+\$a0, \$a0, \$a1
[ 	]+50:[ 	]+14000004[ 	]+lu12i.w[ 	]+\$a0, 0
[ 	]+50: R_LARCH_TLS_LE_HI20[ 	]+TLS1
[ 	]+54:[ 	]+03800084[ 	]+ori[ 	]+\$a0, \$a0, 0x0
[ 	]+54: R_LARCH_TLS_LE_LO12[ 	]+TLS1
[ 	]+58:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+58: R_LARCH_TLS_IE_PC_HI20[ 	]+TLS1
[ 	]+5c:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+5c: R_LARCH_TLS_IE_PC_LO12[ 	]+TLS1
[ 	]+60:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+60: R_LARCH_TLS_IE64_PC_LO20[ 	]+TLS1
[ 	]+64:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+64: R_LARCH_TLS_IE64_PC_HI12[ 	]+TLS1
[ 	]+68:[ 	]+380c1484[ 	]+ldx.d[ 	]+\$a0, \$a0, \$a1
[ 	]+6c:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+6c: R_LARCH_TLS_LD_PC_HI20[ 	]+TLS1
[ 	]+70:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+70: R_LARCH_GOT_PC_LO12[ 	]+TLS1
[ 	]+74:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+74: R_LARCH_GOT64_PC_LO20[ 	]+TLS1
[ 	]+78:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+78: R_LARCH_GOT64_PC_HI12[ 	]+TLS1
[ 	]+7c:[ 	]+00109484[ 	]+add.d[ 	]+\$a0, \$a0, \$a1
[ 	]+80:[ 	]+1a000004[ 	]+pcalau12i[ 	]+\$a0, 0
[ 	]+80: R_LARCH_TLS_GD_PC_HI20[ 	]+TLS1
[ 	]+84:[ 	]+02c00005[ 	]+li.d[ 	]+\$a1, 0
[ 	]+84: R_LARCH_GOT_PC_LO12[ 	]+TLS1
[ 	]+88:[ 	]+16000005[ 	]+lu32i.d[ 	]+\$a1, 0
[ 	]+88: R_LARCH_GOT64_PC_LO20[ 	]+TLS1
[ 	]+8c:[ 	]+030000a5[ 	]+lu52i.d[ 	]+\$a1, \$a1, 0
[ 	]+8c: R_LARCH_GOT64_PC_HI12[ 	]+TLS1
[ 	]+90:[ 	]+00109484[ 	]+add.d[ 	]+\$a0, \$a0, \$a1
