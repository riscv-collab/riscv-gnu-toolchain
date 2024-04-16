	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rax
	.cfi_def_cfa_offset 16
	push    %rbx
	.cfi_def_cfa_offset 24
	.cfi_offset %rbx, -24
	pushq   %rbp
	.cfi_def_cfa_offset 32
	.cfi_offset %rbp, -32
	popq    %rbp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 24
	popq    %rbx
	.cfi_restore %rbx
	.cfi_def_cfa_offset 16
	popq    %rax
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
