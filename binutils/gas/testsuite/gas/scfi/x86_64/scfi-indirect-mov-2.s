# Testcase for movq instructions
	.text
	.globl   foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp

	subq   $16,%rsp

# store rbx at %rsp; rsp = rbp - 16;
	movq   %rbx, -16(%rbp)
	.cfi_rel_offset %rbx, -16
# store r12 at %rsp + 8; rsp = rbp -16;
	movq   %r12, -8(%rbp)
	.cfi_rel_offset %r12, -8

	call bar

	movq   -16(%rbp), %rbx
	.cfi_restore %rbx
	movq   -8(%rbp), %r12
	.cfi_restore %r12

	addq   $16,%rsp

	mov   %rbp, %rsp
	.cfi_def_cfa_register %rsp
	pop   %rbp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
	.size   foo, .-foo
