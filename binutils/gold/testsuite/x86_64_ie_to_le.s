	.text
	.p2align 4
	.globl	_start
	.type	_start, @function
_start:
	addq	foo@gottpoff(%rip), %r12
	movq	foo@gottpoff(%rip), %rax
	addq	foo@gottpoff(%rip), %r16
	movq	foo@gottpoff(%rip), %r20
	.size	_start, .-_start
	.section	.tdata,"awT",@progbits
	.align 4
	.type	foo, @object
	.size	foo, 4
foo:
	.long	30
	.section	.note.GNU-stack,"",@progbits
