# An example of static stack allocation by hand
	.type foo, @function
foo:
	.cfi_startproc
	/* Allocate space for 7 registers.  */
	subq   $56,%rsp
	.cfi_adjust_cfa_offset 56
	movq   %rsp, %rax
	movq   %rbx,(%rax)
	.cfi_rel_offset %rbx, 0
	movq   %rbp,8(%rax)
	.cfi_rel_offset %rbp, 8
	movq   %r12,16(%rax)
	.cfi_rel_offset %r12, 16

	/* Setup parameter for __foo_internal.  */
	/* selfpc is the return address on the stack.  */
	movq   56(%rsp),%rsi
	/* Get frompc via the frame pointer.  */
	movq   8(%rbp),%rdi
	call __foo_internal
	/* Pop the saved registers.  Please note that `foo' has no
	   return value.  */

	movq   16(%rax),%r12
	.cfi_restore %r12
	movq   8(%rax),%rbp
	.cfi_restore %rbp
	movq   (%rsp),%rbx
	.cfi_restore %rbx
	addq   $56,%rsp
	.cfi_adjust_cfa_offset -56
	ret
	.cfi_endproc
	.size foo, .-foo
