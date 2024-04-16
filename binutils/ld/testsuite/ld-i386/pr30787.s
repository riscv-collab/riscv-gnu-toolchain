	.text
	.globl foo
foo:
	jmp	bar@PLT
	leal	func@GOT(%ebx), %eax
	.section .note.GNU-stack,"",@progbits
