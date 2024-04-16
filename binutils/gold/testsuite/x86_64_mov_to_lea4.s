	.text
	.globl	foo
	.hidden foo
	.type	foo, @function
foo:
	ret
	.size	foo, .-foo
	.globl	_start
	.type	_start, @function
_start:
	movq	foo@GOTPCREL(%rip), %rax
	movq	foo@GOTPCREL(%rip), %r26
	.size	_start, .-_start
