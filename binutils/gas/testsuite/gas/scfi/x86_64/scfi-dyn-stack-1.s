	.text
	.globl   foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq   %rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq   %r13
	pushq   %r12
	pushq   %rbx
	subq    $8, %rsp
	.cfi_offset %r13, -24
	.cfi_offset %r12, -32
	.cfi_offset %rbx, -40
	call    read_user
	testl   %eax, %eax
	jle    .L2
	cltq
	xorl   %ebx, %ebx
	leaq   0(,%rax,4), %r13
	.p2align 4,,10
	.p2align 3
.L3:
	leaq   15(%rbx), %rax
	movq   %rsp, %r12
	addq   $4, %rbx
	andq   $-16, %rax
	subq   %rax, %rsp
	movq   %rsp, %rdi
	call   foo
	movq   %r12, %rsp
	cmpq   %r13, %rbx
	jne   .L3
.L2:
	leaq   -24(%rbp), %rsp
	xorl   %eax, %eax
	popq   %rbx
	popq   %r12
	popq   %r13
	popq   %rbp
	.cfi_restore %rbp
	.cfi_def_cfa_register %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
