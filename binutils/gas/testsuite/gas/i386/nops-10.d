#objdump: -drw
#name: nops 10

.*: +file format .*


Disassembly of section .text:

0+ <default>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	2e 8d b4 26 00 00 00 00 	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi
[ 	]*[a-f0-9]+:	2e 8d 74 26 00       	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi
#pass
