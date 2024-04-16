#as: -march=rv64i -mno-relax
#source: fixup-local.s
#objdump: -dr

.*:[ 	]+file format .*

Disassembly of section .text:

0+0000 <foo>:
[ 	]+0:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0
[ 	]+4:[ 	]+00850513[ 	]+addi[ 	]+a0,a0,8 # 8 <foo\+0x8>
[ 	]+8:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0
[ 	]+8:[ 	]+R_RISCV_PCREL_HI20[ 	]+bar.*
[ 	]+c:[ 	]+00050513[ 	]+mv[ 	]+a0,a0
[ 	]+c:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.L0.*
[ 	]+10:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0
[ 	]+10:[ 	]+R_RISCV_PCREL_HI20[ 	]+foo.*
[ 	]+14:[ 	]+00050513[ 	]+mv[ 	]+a0,a0
[ 	]+14:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.L0.*
[ 	]+18:[ 	]+00000517[ 	]+auipc[ 	]a0,0x0
[ 	]+1c:[ 	]+00852503[ 	]+lw[ 	]+a0,8\(a0\) # 20 <foo\+0x20>
[ 	]+20:[ 	]+00008067[ 	]+ret
