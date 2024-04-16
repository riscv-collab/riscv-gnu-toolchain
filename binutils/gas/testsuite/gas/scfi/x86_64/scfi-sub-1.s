# Testcase for sub instruction.
	.text
	.globl   foo
	.type   foo, @function
foo:
	.cfi_startproc
	subq    $120008, %rsp
	.cfi_def_cfa_offset 120016
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
