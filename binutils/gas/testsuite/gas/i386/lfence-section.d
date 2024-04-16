#as: -mlfence-before-indirect-branch=all
#warning_output: lfence-section.e
#objdump: -dw
#name: -mlfence-before-indirect-branch=all w/ section switches

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	f3 ff d0             	repz call \*%eax
 +[a-f0-9]+:	f3 c3                	repz ret
 +[a-f0-9]+:	cc                   	int3
 +[a-f0-9]+:	cc                   	int3
 +[a-f0-9]+:	cc                   	int3

Disassembly of section \.text2:
#pass
