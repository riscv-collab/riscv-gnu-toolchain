# Testcase for save reg ops for callee-saved registers
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %r12
	.cfi_def_cfa_offset 16
	.cfi_offset %r12, -16
	pushq   %r13
	.cfi_def_cfa_offset 24
	.cfi_offset %r13, -24
# The program may use callee-saved registers for its use, and may even
# chose to spill them to stack if necessary.
	addq    %rax, %r13
	subq    $8, %r13
# These two pushq's of callee-saved regs must NOT generate
# .cfi_offset.
	pushq   %r13
	.cfi_def_cfa_offset 32
	pushq   %rax
	.cfi_def_cfa_offset 40
	popq    %rax
	.cfi_def_cfa_offset 32
	popq    %r13
	.cfi_def_cfa_offset 24
# The SCFI machinery keeps track of where the callee-saved registers
# are on the stack.  It generates a restore operation if the stack
# offsets match.
	popq    %r13
	.cfi_restore %r13
	.cfi_def_cfa_offset 16
	popq    %r12
	.cfi_restore %r12
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
