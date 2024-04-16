# Testcase for sub reg, reg instruction.
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
	subq    %rax, %rsp
# SCFI: Stack-pointer manipulation after switching
# to RBP based tracking is OK.
	addq   %rax, %rsp
# Other kind of sub instructions should not error out in the
# x86_64 -> ginsn translator
	subq    (%r12), %rax
	subq    $1,(%rdi)
	subq    %rax, 40(%r12)
	subl    %edx, -32(%rsp)
	leave
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
