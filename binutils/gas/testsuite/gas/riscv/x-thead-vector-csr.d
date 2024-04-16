#as: -march=rv32if_xtheadvector
#objdump: -dr

.*:[ 	]+file format .*


Disassembly of section .text:

0+000 <.text>:
[ 	]+[0-9a-f]+:[ 	]+00802573[ 	]+csrr[ 	]+a0,th.vstart
[ 	]+[0-9a-f]+:[ 	]+00902573[ 	]+csrr[ 	]+a0,th.vxsat
[ 	]+[0-9a-f]+:[ 	]+00a02573[ 	]+csrr[ 	]+a0,th.vxrm
[ 	]+[0-9a-f]+:[ 	]+c2002573[ 	]+csrr[ 	]+a0,th.vl
[ 	]+[0-9a-f]+:[ 	]+c2102573[ 	]+csrr[ 	]+a0,th.vtype
[ 	]+[0-9a-f]+:[ 	]+c2202573[ 	]+csrr[ 	]+a0,th.vlenb
[ 	]+[0-9a-f]+:[ 	]+00851073[ 	]+csrw[ 	]+th.vstart,a0
[ 	]+[0-9a-f]+:[ 	]+00951073[ 	]+csrw[ 	]+th.vxsat,a0
[ 	]+[0-9a-f]+:[ 	]+00a51073[ 	]+csrw[ 	]+th.vxrm,a0
[ 	]+[0-9a-f]+:[ 	]+c2051073[ 	]+csrw[ 	]+th.vl,a0
[ 	]+[0-9a-f]+:[ 	]+c2151073[ 	]+csrw[ 	]+th.vtype,a0
[ 	]+[0-9a-f]+:[ 	]+c2251073[ 	]+csrw[ 	]+th.vlenb,a0
