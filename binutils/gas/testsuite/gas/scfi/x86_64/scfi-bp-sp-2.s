# Testcase for switching between sp/fp based CFA.
# Although there is stack usage between push %rbp and mov %rsp, %rbp,
# this is a valid ABI/calling convention complaint pattern; It ought to
# work for SCFI.
	.text
	.globl   foo
	.type    foo, @function
foo:
	.cfi_startproc
	pushq   %r14
	.cfi_def_cfa_offset 16
	.cfi_offset %r14, -16
	pushq   %r13
	.cfi_def_cfa_offset 24
	.cfi_offset %r13, -24
	pushq   %r12
	.cfi_def_cfa_offset 32
	.cfi_offset %r12, -32
	pushq   %rbp
	.cfi_def_cfa_offset 40
	.cfi_offset %rbp, -40
	pushq   %rbx
	.cfi_def_cfa_offset 48
	.cfi_offset %rbx, -48
	movq    %rdi, %rbx
	subq    $32, %rsp
	.cfi_def_cfa_offset 80
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
	xorl    %eax, %eax
	addq    $32, %rsp
	popq    %rbx
	.cfi_restore %rbx
# The SCFI machinery must be able to figure out the offset for CFA
# as it switches back to REG_SP based tracking after this instruction.
	popq    %rbp
	.cfi_def_cfa_register %rsp
	.cfi_def_cfa_offset 40
	.cfi_restore %rbp
	.cfi_def_cfa_offset 32
	popq    %r12
	.cfi_restore %r12
	.cfi_def_cfa_offset 24
	popq    %r13
	.cfi_restore %r13
	.cfi_def_cfa_offset 16
	popq    %r14
	.cfi_restore %r14
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
	.size   foo, .-foo
