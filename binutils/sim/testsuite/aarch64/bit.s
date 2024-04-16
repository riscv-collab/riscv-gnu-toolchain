# mach: aarch64

# Check the bitwise vector instructions: bif, bit, bsl, eor.

.include "testutils.inc"

	.data
	.align 4
inputa:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
inputb:
	.word 0x40302010
	.word 0x80706050
	.word 0xc0b0a090
	.word 0x01f0e0d0
mask:
	.word 0xFF00FF00
	.word 0x00FF00FF
	.word 0xF0F0F0F0
	.word 0x0F0F0F0F

	start
	adrp x0, inputa
	ldr q0, [x0, #:lo12:inputa]
	adrp x0, inputb
	ldr q1, [x0, #:lo12:inputb]
	adrp x0, mask
	ldr q2, [x0, #:lo12:mask]

	mov v3.8b, v0.8b
	bif v3.8b, v1.8b, v2.8b
	addv b4, v3.8b
	mov x1, v4.d[0]
	cmp x1, #50
	bne .Lfailure

	mov v3.16b, v0.16b
	bif v3.16b, v1.16b, v2.16b
	addv b4, v3.16b
	mov x1, v4.d[0]
	cmp x1, #252
	bne .Lfailure

	mov v3.8b, v0.8b
	bit v3.8b, v1.8b, v2.8b
	addv b4, v3.8b
	mov x1, v4.d[0]
	cmp x1, #50
	bne .Lfailure

	mov v3.16b, v0.16b
	bit v3.16b, v1.16b, v2.16b
	addv b4, v3.16b
	mov x1, v4.d[0]
	cmp x1, #13
	bne .Lfailure

	mov v3.8b, v2.8b
	bsl v3.8b, v0.8b, v1.8b
	addv b4, v3.8b
	mov x1, v4.d[0]
	cmp x1, #50
	bne .Lfailure

	mov v3.16b, v2.16b
	bsl v3.16b, v0.16b, v1.16b
	addv b4, v3.16b
	mov x1, v4.d[0]
	cmp x1, #252
	bne .Lfailure

	mov v3.8b, v0.8b
	eor v3.8b, v1.8b, v2.8b
	addv b4, v3.8b
	mov x1, v4.d[0]
	cmp x1, #252
	bne .Lfailure

	mov v3.16b, v0.16b
	eor v3.16b, v1.16b, v2.16b
	addv b4, v3.16b
	mov x1, v4.d[0]
	cmp x1, #247
	bne .Lfailure

	pass
.Lfailure:
	fail
