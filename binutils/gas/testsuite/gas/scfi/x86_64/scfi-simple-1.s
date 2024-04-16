# Simple test
# A wierd function, but SCFI machinery does not complain yet.
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo

