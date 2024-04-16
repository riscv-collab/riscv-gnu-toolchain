#objdump: -drw
#name: i386 nops 9

.*: +file format .*

Disassembly of section .text:

0+ <default>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	2e 8d b4 26 00 00 00 00 	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi
[ 	]*[a-f0-9]+:	2e 8d 74 26 00       	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi

0+10 <nopopcnt>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	2e 8d b4 26 00 00 00 00 	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi
[ 	]*[a-f0-9]+:	2e 8d 74 26 00       	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi

0+20 <popcnt>:
[ 	]*[a-f0-9]+:	f3 0f b8 f0          	popcnt %eax,%esi
[ 	]*[a-f0-9]+:	2e 8d b4 26 00 00 00 00 	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi
[ 	]*[a-f0-9]+:	8d 74 26 00          	lea    (0x)?0\(%esi,%eiz,1\),%esi

0+30 <nop>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 2e 0f 1f 84 00 00 00 00 00 	data16 nopw %cs:0x0\(%eax,%eax,1\)
[ 	]*[a-f0-9]+:	66 90                	xchg   %ax,%ax
#pass
