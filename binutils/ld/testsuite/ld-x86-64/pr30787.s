	.text
	.globl foo
foo:
	jmp	bar@PLT
	movq	func@GOTPCREL(%rip), %rax
	.section .note.GNU-stack,"",@progbits
