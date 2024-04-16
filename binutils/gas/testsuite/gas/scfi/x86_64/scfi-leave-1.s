# Testcase for leave insn
	.text
	.globl   foo
	.type    foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
	push    %rbx
	.cfi_offset %rbx, -24
	push    %rdi
	pop     %rdi
	pop     %rbx
	.cfi_restore %rbx
	leave
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
