# The first version medium codel model function call is: pcalau12i + jirl.
# R_LARCH_PCALA_HI20 only need to generate PLT entry for function symbols.
	.text
	.globl	a

	.data
	.align	2
	.type	a, @object
	.size	a, 4
a:
	.word	1

	.text
	.align	2
	.globl	test
	.type	test, @function
test:
	pcalau12i	$r12,%pc_hi20(a)
	ld.w	$r12,$r12,%pc_lo12(a)
	.size	test, .-test
