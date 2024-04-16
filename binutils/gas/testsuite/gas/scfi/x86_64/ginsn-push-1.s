## Testcase with a variety of push.
## all push insns valid in 64-bit mode must be processed for SCFI.
	.text
	.globl  foo
	.type   foo, @function
foo:
	pushw   %fs
	pushw   %gs
	pushw   $40
	pushw   -8(%r10)
	pushq   -8(,%r10)
	pushfw
	push    symbol
	push    %rax
	ret
.LFE0:
	.size   foo, .-foo
