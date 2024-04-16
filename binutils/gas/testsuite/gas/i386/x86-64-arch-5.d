#objdump: -dw
#name: x86-64 arch 5

.*:     file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:[ 	]*c4 c2 59 50 d4[ 	]*\{vex\} vpdpbusd %xmm12,%xmm4,%xmm2
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri %rax,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*62 f2 6f 48 68 d9[ 	]*vp2intersectd %zmm1,%zmm2,%k3
[ 	]*[a-f0-9]+:[ 	]*0f 18 3d 78 56 34 12[ 	]*prefetchit0 0x12345678\(%rip\)        # .*
#pass
