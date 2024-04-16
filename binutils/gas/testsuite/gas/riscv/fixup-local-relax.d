#as: -march=rv64i -mrelax
#source: fixup-local.s
#objdump: -dr

.*:[ 	]+file format .*


Disassembly of section .text:

0+0000 <foo>:
[ 	]+0:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0
[ 	]+0:[ 	]+R_RISCV_PCREL_HI20[ 	]+.LL0.*
[ 	]+0:[ 	]+R_RISCV_RELAX.*
[ 	]+4:[ 	]+00850513[ 	]+addi[ 	]+a0,a0,8 # 8 <.LL0>
[ 	]+4:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.L0.*
[ 	]+4:[ 	]+R_RISCV_RELAX.*

0+0008 <.LL0>:
[ 	]+8:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0
[ 	]+8:[ 	]+R_RISCV_PCREL_HI20[ 	]+bar.*
[ 	]+8:[ 	]+R_RISCV_RELAX.*
[ 	]+c:[ 	]+00050513[ 	]+mv[ 	]+a0,a0
[ 	]+c:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.L0.*
[ 	]+c:[ 	]+R_RISCV_RELAX.*
[ 	]+10:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0
[ 	]+10:[ 	]+R_RISCV_PCREL_HI20[ 	]+foo.*
[ 	]+10:[ 	]+R_RISCV_RELAX.*
[ 	]+14:[ 	]+00050513[ 	]+mv[ 	]+a0,a0
[ 	]+14:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.L0.*
[ 	]+14:[ 	]+R_RISCV_RELAX.*

0+0018 <.LL1>:
[ 	]+18:[ 	]+00000517[ 	]+auipc[ 	]a0,0x0
[ 	]+18:[ 	]+R_RISCV_PCREL_HI20[ 	]+.LL2.*
[ 	]+18:[ 	]R_RISCV_RELAX.*
[ 	]+1c:[ 	]+00852503[ 	]+lw[ 	]+a0,8\(a0\) # 20 <.LL2>
[ 	]+1c:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.LL1.*
[ 	]+1c:[ 	]+R_RISCV_RELAX.*

0+0020 <.LL2>:
[ 	]+20:[ 	]+00008067[ 	]+ret
