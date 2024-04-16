# Testcase for a diagnostic around asymetrical restore
# Testcase inspired by byte_insert_op1 in libiberty
# False positive for the diagnostic
	.type   foo, @function
foo:
.LFB10:
	.cfi_startproc
	endbr64
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq   %rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq   %r12
	pushq   %rbx
	subq   $24, %rsp
	.cfi_offset %r12, -24
	.cfi_offset %rbx, -32
	movl   %edi, -20(%rbp)
	movq   %rsi, -32(%rbp)
	movl   %edx, -24(%rbp)
	movq   %rcx, -40(%rbp)
# The assembler cannot differentiate that the following
# mov to %rbx is not a true restore operation, but simply
# %rbx register usage as a scratch reg of some sort.
# The assembler merely warns of a possible asymetric restore operation
# In this case, its noise for the user unfortunately.
	movq   -40(%rbp), %rbx
	movq   -40(%rbp), %rax
	leaq   3(%rax), %r12
	jmp   .L563
.L564:
	subq   $1, %rbx
	subq   $1, %r12
	movzbl   (%rbx), %eax
	movb   %al, (%r12)
.L563:
	cmpq   -32(%rbp), %rbx
	jne   .L564
	movl   -24(%rbp), %edx
	movq   -32(%rbp), %rcx
	movl   -20(%rbp), %eax
	movq   %rcx, %rsi
	movl   %eax, %edi
	call   byte_store_op1
	nop
	addq   $24, %rsp
	popq   %rbx
	popq   %r12
	popq   %rbp
	.cfi_def_cfa %rsp, 8
	ret
	.cfi_endproc
.LFE10:
	.size   foo, .-foo
