#as: -march=rv32i_xtheadfmv
#source: x-thead-fmv.s
#objdump: -dr

.*:[ 	]+file format .*

Disassembly of section .text:

0+000 <target>:
[ 	]+[0-9a-f]+:[ 	]+a005158b[ 	]+th.fmv.hw.x[ 	]+fa1,a0
[ 	]+[0-9a-f]+:[ 	]+c005158b[ 	]+th.fmv.x.hw[ 	]+a1,fa0
