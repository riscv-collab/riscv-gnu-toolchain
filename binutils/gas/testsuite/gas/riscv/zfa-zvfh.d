#as: -march=rv32iq2p2_zfa_zvfh
#objdump: -d

.*:[ 	]+file format .*

Disassembly of section .text:

0+000 <target>:
[ 	]+[0-9a-f]+:[ 	]+f41c00d3[ 	]+fli\.h[ 		]+ft1,0x1p\+3
[ 	]+[0-9a-f]+:[ 	]+f41c80d3[ 	]+fli\.h[ 		]+ft1,0x1p\+4
[ 	]+[0-9a-f]+:[ 	]+f41d00d3[ 	]+fli\.h[ 		]+ft1,0x1p\+7
[ 	]+[0-9a-f]+:[ 	]+f41d80d3[ 	]+fli\.h[ 		]+ft1,0x1p\+8
[ 	]+[0-9a-f]+:[ 	]+f41e00d3[ 	]+fli\.h[ 		]+ft1,0x1p\+15
[ 	]+[0-9a-f]+:[ 	]+f41e80d3[ 	]+fli\.h[ 		]+ft1,0x1p\+16
[ 	]+[0-9a-f]+:[ 	]+f41f00d3[ 	]+fli\.h[ 		]+ft1,inf
[ 	]+[0-9a-f]+:[ 	]+f41f80d3[ 	]+fli\.h[ 		]+ft1,nan
