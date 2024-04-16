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
	movl	foo@GOTPCREL+4(%rip), %eax
	movl	foo@GOTPCREL+4(%rip), %r26d
	.size	_start, .-_start
