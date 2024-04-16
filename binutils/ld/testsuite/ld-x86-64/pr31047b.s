	.text
	.globl _start
_start:
	.nop
	.reloc 0, BFD_RELOC_NONE, foo
	.section .note.GNU-stack,"",@progbits
