	.text
	.globl	foo
	.type	foo, @function
foo:
	call	bar@PLT
	ret
	.section	.note.GNU-stack,"",@progbits
