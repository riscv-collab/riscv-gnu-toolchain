## Testcase with a variety of lea.
## This test is run with -O2 by default to check
## SCFI in wake of certain target optimizations.
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
# This lea gets transformed to mov %rsp, %rbp when -O2.
# The SCFI machinery must see it as such.
	lea    (%rsp), %rbp
	.cfi_def_cfa_register %rbp
	push   %r13
	.cfi_offset %r13, -24
	subq   $8, %rsp
	testl  %eax, %eax
	jle   .L2
.L3:
	movq   %rsp, %r12
	lea    -0x2(%r13),%rax
	lea    0x8(%r12,%rdx,4),%r8
	movq   %r12, %rsp
	jne   .L3
.L2:
	leaq   -8(%rbp), %rsp
	xorl   %eax, %eax
	popq   %r13
	.cfi_restore %r13
	popq   %rbp
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
