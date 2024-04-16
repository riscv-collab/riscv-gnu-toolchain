# Testcase for switching between sp/fp based CFA.
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
	addq    %rax, %rdi
	mov     %rbp, %rsp
	.cfi_def_cfa_register %rsp
	pop     %rbp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
