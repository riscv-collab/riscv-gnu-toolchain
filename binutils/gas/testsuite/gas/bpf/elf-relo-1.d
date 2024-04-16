#as: -EL -mdialect=normal
#objdump: -tdr
#source elf-relo-1.s
#name: eBPF ELF relocations 1

.*: +file format elf64-bpfle

SYMBOL TABLE:
0000000000000000 l    d  .text	0000000000000000 .text
0000000000000000 l    d  .data	0000000000000000 .data
0000000000000000 l    d  .bss	0000000000000000 .bss
0000000000000006 l       .data	0000000000000000 bar
0000000000000000 l     F .text	0000000000000000 baz
0000000000000030 l     F .text	0000000000000000 qux
0000000000000004 g       .data	0000000000000000 foo
0000000000000000         \*UND\*	0000000000000000 somefunc



Disassembly of section .text:

0+ <baz>:
   0:	18 01 00 00 00 00 00 00 	lddw %r1,0
   8:	00 00 00 00 00 00 00 00 
			0: R_BPF_64_64	foo
  10:	b7 02 00 00 06 00 00 00 	mov %r2,6
			14: R_BPF_64_ABS32	.data
  18:	18 03 00 00 30 00 00 00 	lddw %r3,48
  20:	00 00 00 00 00 00 00 00 
			18: R_BPF_64_64	.text
  28:	85 10 00 00 ff ff ff ff 	call -1
			28: R_BPF_64_32	somefunc

0+30 <qux>:
  30:	95 00 00 00 00 00 00 00 	exit
