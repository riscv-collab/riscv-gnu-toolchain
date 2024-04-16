#source: pr31047a.s
#source: pr31047b.s
#as: --64
#ld: -pie -melf_x86_64
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

[a-f0-9]+ <_start>:
 +[a-f0-9]+:	90                   	nop
