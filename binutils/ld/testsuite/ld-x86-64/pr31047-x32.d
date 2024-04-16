#source: pr31047a.s
#source: pr31047b.s
#as: --x32
#ld: -pie -melf32_x86_64
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

[a-f0-9]+ <_start>:
 +[a-f0-9]+:	90                   	nop
