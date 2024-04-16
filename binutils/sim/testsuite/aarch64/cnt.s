# mach: aarch64

# Check the popcount instruction: cnt.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x0f070605
	.word 0x44332211
	.word 0xff776655

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	cnt v1.8b, v0.8b
	addv b2, v1.8b
	mov x1, v2.d[0]
	cmp x1, #16
	bne .Lfailure

	cnt v1.16b, v0.16b
	addv b2, v1.16b
	mov x1, v2.d[0]
	cmp x1, #48
	bne .Lfailure

	pass
.Lfailure:
	fail
