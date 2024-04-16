	.type byte_insert_op1, @function
byte_insert_op1:
.LFB10:
	.cfi_startproc
	endbr64
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq   %rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq   %r12
	.cfi_offset %r12, -24
	pushq   %rbx
	.cfi_offset %rbx, -32
	subq   $24, %rsp
	movl   %edi, -20(%rbp)
	movq   %rsi, -32(%rbp)
	movl   %edx, -24(%rbp)
	movq   %rcx, -40(%rbp)
# The program may use callee-saved registers for its use, and may even
# chose to read them from stack if necessary.  The following use should
# not be treated as reg restore for SCFI purposes (because rbx has been
# saved to -16(%rbp).
	movq   -40(%rbp), %rbx
	movq   -40(%rbp), %rax
	leaq   3(%rax), %r12
	jmp   .L563
.L564:
	subq   $1, %rbx
	subq   $1, %r12
	movzbl (%rbx), %eax
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
	.cfi_restore %rbx
	popq   %r12
	.cfi_restore %r12
	popq   %rbp
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE10:
	.size   byte_insert_op1, .-byte_insert_op1
