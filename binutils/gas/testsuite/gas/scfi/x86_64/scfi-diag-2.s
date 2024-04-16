# Testcase for a diagnostic around asymetrical restore
	.type   foo, @function
foo:
.LFB10:
	.cfi_startproc
	endbr64
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq   %r12
	pushq   %rbx
	subq    $24, %rsp
	.cfi_offset %r12, -24
	.cfi_offset %rbx, -32
	addq    $24, %rsp
# Note that the order of r12 and rbx restore does not match
# order of the corresponding save(s).
# The SCFI machinery warns the user.
	popq    %r12
	popq    %rbx
	popq    %rbp
	.cfi_def_cfa %rsp, 8
	ret
	.cfi_endproc
.LFE10:
	.size   foo, .-foo
