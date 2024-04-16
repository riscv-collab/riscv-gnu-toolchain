#source: ../ld-x86-64/pr31047a.s
#source: ../ld-x86-64/pr31047b.s
#as: --32
#ld: -pie -melf_i386
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

[a-f0-9]+ <_start>:
 +[a-f0-9]+:	90                   	nop
