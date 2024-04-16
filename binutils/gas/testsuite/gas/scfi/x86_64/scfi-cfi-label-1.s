# Testcase for .cfi_label directives
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	nop
	.globl cfi1
	.cfi_label cfi1
	nop
	.cfi_label cfi2
	nop
	.cfi_label .Lcfi3
	addq    $8, %rsp
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
