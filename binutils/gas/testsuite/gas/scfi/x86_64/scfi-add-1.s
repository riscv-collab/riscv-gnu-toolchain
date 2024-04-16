# Testcase for add instruction.
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	addq    $8, %rsp
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo

