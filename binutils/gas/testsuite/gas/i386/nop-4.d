#objdump: -drw
#name: i386 .nops 4

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	31 c0                	xor    %eax,%eax
 +[a-f0-9]+:	85 c0                	test   %eax,%eax
 +[a-f0-9]+:	2e 8d b4 26 00 00 00 00 	lea    %cs:(0x)?0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	31 c0                	xor    %eax,%eax
 +[a-f0-9]+:	31 c0                	xor    %eax,%eax

Disassembly of section .altinstr_replacement:

0+ <.altinstr_replacement>:
 +[a-f0-9]+:	89 c0                	mov    %eax,%eax
 +[a-f0-9]+:	89 c0                	mov    %eax,%eax
 +[a-f0-9]+:	89 c0                	mov    %eax,%eax
 +[a-f0-9]+:	e9 fc ff ff ff       	jmp    7 <.altinstr_replacement\+0x7>	7: (R_386_PC)?(DISP)?32	foo
#pass
