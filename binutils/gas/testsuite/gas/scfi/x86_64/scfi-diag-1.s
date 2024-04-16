# Testcase for REG_FP based CFA
# and using REG_FP as scratch.
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
# The following add causes REG_FP to become untraceable
	addq    %rax, %rbp
# CFA cannot be recovered via REG_FP anymore
	pop     %rbp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
