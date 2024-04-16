#objdump: -drw
#name: x86-64 nops 6

.*: +file format .*


Disassembly of section .text:

0+ <default>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 2e 0f 1f 84 00 00 00 00 00 	data16 cs nopw (0x)?0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	66 90                	xchg   %ax,%ax
#pass
