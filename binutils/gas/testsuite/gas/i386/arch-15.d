#objdump: -dw
#name: i386 arch 15

.*:     file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:[ 	]*c4 e2 59 50 d2[ 	]*\{vex\} vpdpbusd %xmm2,%xmm4,%xmm2
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%ecx\)
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*62 f2 6f 48 68 d9[ 	]*vp2intersectd %zmm1,%zmm2,%k3
#pass
