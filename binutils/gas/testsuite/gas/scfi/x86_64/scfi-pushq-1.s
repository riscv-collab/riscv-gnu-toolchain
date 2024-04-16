## Testcase with a variety of pushq.
## all push insns valid in 64-bit mode must be processed for SCFI.
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rax
	.cfi_def_cfa_offset 16
	pushq   16(%rax)
	.cfi_def_cfa_offset 24
	push   $1048576
	.cfi_def_cfa_offset 32
	pushq   %fs
	.cfi_def_cfa_offset 40
	pushq   %gs
	.cfi_def_cfa_offset 48
	pushf
	.cfi_def_cfa_offset 56
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
