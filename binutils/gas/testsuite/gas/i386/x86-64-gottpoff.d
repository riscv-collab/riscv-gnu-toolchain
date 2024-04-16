#as:
#objdump: -dwr
#name: x86-64 gottpoff

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	48 03 05 00 00 00 00 	add    0x0\(%rip\),%rax        # 7 <_start\+0x7>	3: R_X86_64_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	48 8b 05 00 00 00 00 	mov    0x0\(%rip\),%rax        # e <_start\+0xe>	a: R_X86_64_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	d5 48 03 05 00 00 00 00 	add    0x0\(%rip\),%r16        # 16 <_start\+0x16>	12: R_X86_64_CODE_4_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	d5 48 8b 25 00 00 00 00 	mov    0x0\(%rip\),%r20        # 1e <_start\+0x1e>	1a: R_X86_64_CODE_4_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	48 03 05 00 00 00 00 	add    0x0\(%rip\),%rax        # 25 <_start\+0x25>	21: R_X86_64_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	48 8b 05 00 00 00 00 	mov    0x0\(%rip\),%rax        # 2c <_start\+0x2c>	28: R_X86_64_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	d5 48 03 05 00 00 00 00 	add    0x0\(%rip\),%r16        # 34 <_start\+0x34>	30: R_X86_64_CODE_4_GOTTPOFF	foo-0x4
 +[a-f0-9]+:	d5 48 8b 25 00 00 00 00 	mov    0x0\(%rip\),%r20        # 3c <_start\+0x3c>	38: R_X86_64_CODE_4_GOTTPOFF	foo-0x4
#pass
