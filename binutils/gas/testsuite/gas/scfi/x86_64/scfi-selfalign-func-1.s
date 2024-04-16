# If it is known that the caller of self_aligning_foo may have had
# the stack pointer unaligned to 16-bytes boundary,  such self-aligning
# functions may be used by asm programmers.
	.globl  self_aligning_foo
	.type   self_aligning_foo, @function
self_aligning_foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
# The following 'and' op aligns the stack pointer.
# At the same time, it causes REG_SP to become non-traceable
# for SCFI purposes.  But no warning is issued as no further stack
# size tracking is needed for SCFI purposes.
	andq    $-16, %rsp
	subq    $32, %rsp
	movl    %edi, 12(%rsp)
	movl    %esi, 8(%rsp)
	movl    $0, %eax
	call    vector_using_function
	movaps  %xmm0, 16(%rsp)
	movl    12(%rsp), %edx
	movl    8(%rsp), %eax
	addl    %edx, %eax
	leave
# GCC typically generates a '.cfi_def_cfa 7, 8' for leave
# insn.  The SCFI however, will generate the following:
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   self_aligning_foo, .-self_aligning_foo
