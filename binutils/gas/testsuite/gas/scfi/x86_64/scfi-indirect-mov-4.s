# Testcase for save/unsave of callee-saved registers
# Must be run -W as there is an expected warning as
# noted below
	.type   foo, @function
foo:
.LFB118:
	.cfi_startproc
	pushq   %r15
	.cfi_def_cfa_offset 16
	.cfi_offset %r15, -16
	pushq   %r14
	.cfi_def_cfa_offset 24
	.cfi_offset %r14, -24
	movl    %r8d, %r14d
	pushq   %r13
	.cfi_def_cfa_offset 32
	.cfi_offset %r13, -32
	movq    %rdi, %r13
	pushq   %r12
	.cfi_def_cfa_offset 40
	.cfi_offset %r12, -40
	movq    %rsi, %r12
	pushq   %rbp
	.cfi_def_cfa_offset 48
	.cfi_offset %rbp, -48
	movq    %rcx, %rbp
	pushq   %rbx
	.cfi_def_cfa_offset 56
	.cfi_offset %rbx, -56
	movq   %rdx, %rbx
	subq   $40, %rsp
	.cfi_def_cfa_offset 96
	testb   $1, 37(%rdx)
	je   .L2
.L3:
# The following is not a restore of r15: rbp has been used as
# scratch register already.  The SCFI machinery must know that
# REG_FP is not traceable.
# A warning here is expected:
# 41: Warning: SCFI: asymetrical register restore
	movq   32(%rbp), %r15
	cmpq   $0, 64(%r15)
	je   .L2
.L2:
	addq   $40, %rsp
	.cfi_def_cfa_offset 56
	popq   %rbx
	.cfi_restore %rbx
	.cfi_def_cfa_offset 48
	popq   %rbp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 40
	popq   %r12
	.cfi_restore %r12
	.cfi_def_cfa_offset 32
	popq   %r13
	.cfi_restore %r13
	.cfi_def_cfa_offset 24
	popq   %r14
	.cfi_restore %r14
	.cfi_def_cfa_offset 16
	popq   %r15
	.cfi_restore %r15
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE118:
	.size   foo, .-foo
