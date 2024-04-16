# Testcase for pushsection directive and SCFI.
# The .pushsection directive must cause creation of a new FDE.
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
	subq    $8, %rsp
	.cfi_def_cfa_offset 32
	mov   %rax, %rbx
	.pushsection .text2
# It's the .type directive here that enforces SCFI generation
# for the code block that follows
	.type   foo2, @function
foo2:
	.cfi_startproc
	subq    $40, %rsp
	.cfi_def_cfa_offset 48
	ret
	.cfi_endproc
	.size foo2, .-foo2
	.popsection
	addq   $8, %rsp
	.cfi_def_cfa_offset 24
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
