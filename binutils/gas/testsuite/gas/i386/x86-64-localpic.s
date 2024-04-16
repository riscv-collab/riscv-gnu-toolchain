	.text
foo:
	.quad 0
	movq	foo@GOTPCREL(%rip), %rax
	movq	bar@GOTPCREL(%rip), %rax
	movq	foo@GOTPCREL(%rip), %r26
	movq	bar@GOTPCREL(%rip), %r26
	bar = 0xfffffff0
