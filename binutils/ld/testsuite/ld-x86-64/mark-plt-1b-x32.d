#source: mark-plt-1.s
#as: --x32
#ld: -melf32_x86_64 -shared -z mark-plt --hash-style=both
#objdump: -dw

#...
0+1010 <bar@plt>:
 +1010:	ff 25 9a 10 00 00    	jmp    \*0x109a\(%rip\)        # 20b0 <bar>
 +1016:	68 00 00 00 00       	push   \$0x0
 +101b:	e9 e0 ff ff ff       	jmp    1000 <bar@plt-0x10>

Disassembly of section .text:

0+1020 <foo>:
 +1020:	e8 eb ff ff ff       	call   1010 <bar@plt>
#pass
