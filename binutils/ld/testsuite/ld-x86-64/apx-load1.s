	.data
	.type	bar, @object
bar:
	.byte	1
	.size	bar, .-bar
	.globl	foo
	.type	foo, @object
foo:
	.byte	1
	.size	foo, .-foo
	.text
	.globl	_start
	.type	_start, @function
_start:
	adcl	bar@GOTPCREL(%rip), %r16d
	addl	bar@GOTPCREL(%rip), %r17d
	andl	bar@GOTPCREL(%rip), %r18d
	cmpl	bar@GOTPCREL(%rip), %r19d
	orl	bar@GOTPCREL(%rip), %r20d
	sbbl	bar@GOTPCREL(%rip), %r21d
	subl	bar@GOTPCREL(%rip), %r22d
	xorl	bar@GOTPCREL(%rip), %r23d
	testl	%r24d, bar@GOTPCREL(%rip)
	adcq	bar@GOTPCREL(%rip), %r16
	addq	bar@GOTPCREL(%rip), %r17
	andq	bar@GOTPCREL(%rip), %r18
	cmpq	bar@GOTPCREL(%rip), %r19
	orq	bar@GOTPCREL(%rip), %r20
	sbbq	bar@GOTPCREL(%rip), %r21
	subq	bar@GOTPCREL(%rip), %r22
	xorq	bar@GOTPCREL(%rip), %r23
	testq	%r24, bar@GOTPCREL(%rip)
	adcl	foo@GOTPCREL(%rip), %r16d
	addl	foo@GOTPCREL(%rip), %r17d
	andl	foo@GOTPCREL(%rip), %r18d
	cmpl	foo@GOTPCREL(%rip), %r19d
	orl	foo@GOTPCREL(%rip), %r20d
	sbbl	foo@GOTPCREL(%rip), %r21d
	subl	foo@GOTPCREL(%rip), %r22d
	xorl	foo@GOTPCREL(%rip), %r23d
	testl	%r24d, foo@GOTPCREL(%rip)
	adcq	foo@GOTPCREL(%rip), %r16
	addq	foo@GOTPCREL(%rip), %r17
	andq	foo@GOTPCREL(%rip), %r18
	cmpq	foo@GOTPCREL(%rip), %r19
	orq	foo@GOTPCREL(%rip), %r20
	sbbq	foo@GOTPCREL(%rip), %r21
	subq	foo@GOTPCREL(%rip), %r22
	xorq	foo@GOTPCREL(%rip), %r23
	testq	%r24, foo@GOTPCREL(%rip)
	.size	_start, .-_start
