#source: mark-plt-1.s
#as: --64
#ld: -melf_x86_64 -shared -z mark-plt -z ibtplt --hash-style=both
#objdump: -dw

#...
0+1020 <bar@plt>:
 +1020:	f3 0f 1e fa          	endbr64
 +1024:	ff 25 1e 11 00 00    	jmp    \*0x111e\(%rip\)        # 2148 <bar>
 +102a:	66 0f 1f 44 00 00    	nopw   0x0\(%rax,%rax,1\)

Disassembly of section .text:

0+1030 <foo>:
 +1030:	e8 eb ff ff ff       	call   1020 <bar@plt>
#pass
