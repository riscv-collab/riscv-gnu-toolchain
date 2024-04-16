#as:
#objdump: -dwr
#name: x86-64 tlsdesc

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	48 8d 05 00 00 00 00 	lea    0x0\(%rip\),%rax        # 7 <_start\+0x7>	3: R_X86_64_GOTPC32_TLSDESC	foo-0x4
 +[a-f0-9]+:	d5 48 8d 05 00 00 00 00 	lea    0x0\(%rip\),%r16        # f <_start\+0xf>	b: R_X86_64_CODE_4_GOTPC32_TLSDESC	foo-0x4
 +[a-f0-9]+:	d5 48 8d 25 00 00 00 00 	lea    0x0\(%rip\),%r20        # 17 <_start\+0x17>	13: R_X86_64_CODE_4_GOTPC32_TLSDESC	foo-0x4
 +[a-f0-9]+:	48 8d 05 00 00 00 00 	lea    0x0\(%rip\),%rax        # 1e <_start\+0x1e>	1a: R_X86_64_GOTPC32_TLSDESC	foo-0x4
 +[a-f0-9]+:	d5 48 8d 05 00 00 00 00 	lea    0x0\(%rip\),%r16        # 26 <_start\+0x26>	22: R_X86_64_CODE_4_GOTPC32_TLSDESC	foo-0x4
 +[a-f0-9]+:	d5 48 8d 25 00 00 00 00 	lea    0x0\(%rip\),%r20        # 2e <_start\+0x2e>	2a: R_X86_64_CODE_4_GOTPC32_TLSDESC	foo-0x4
#pass
