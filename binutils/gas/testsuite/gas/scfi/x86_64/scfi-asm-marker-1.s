# Testcase where a user may define hot and cold areas of function
# Note how the .type, and .size directives may be placed differently
# than a regular function.

	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	testl   %edi, %edi
	je      .L3
	movl    b(%rip), %eax
	ret
	.cfi_endproc
	.section        .text.unlikely
	.cfi_startproc
	.type   foo.cold, @function
foo.cold:
.L3:
	pushq   %rax
	.cfi_def_cfa_offset 16
	call    abort
	.cfi_endproc
.LFE11:
	.text
	.size   foo, .-foo
	.section        .text.unlikely
	.size   foo.cold, .-foo.cold
