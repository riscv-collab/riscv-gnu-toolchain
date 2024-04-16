# mach: aarch64

# Check the extract narrow instructions: xtn, xtn2.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
input2:
	.word 0x14131211
	.word 0x18171615
	.word 0x1c1b1a19
	.word 0x201f1e1d
x16b:
	.word 0x07050301
	.word 0x0f0d0b09
	.word 0x17151311
	.word 0x1f1d1b19
x8h:
	.word 0x06050201
	.word 0x0e0d0a09
	.word 0x16151211
	.word 0x1e1d1a19
x4s:
	.word 0x04030201
	.word 0x0c0b0a09
	.word 0x14131211
	.word 0x1c1b1a19

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]
	adrp x0, input2
	ldr q1, [x0, #:lo12:input2]

	xtn v2.8b, v0.8h
	xtn2 v2.16b, v1.8h
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, x16b
	ldr x4, [x3, #:lo12:x16b]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:x16b+8]
	cmp x2, x5
	bne .Lfailure

	xtn v2.4h, v0.4s
	xtn2 v2.8h, v1.4s
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, x8h
	ldr x4, [x3, #:lo12:x8h]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:x8h+8]
	cmp x2, x5
	bne .Lfailure

	xtn v2.2s, v0.2d
	xtn2 v2.4s, v1.2d
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, x4s
	ldr x4, [x3, #:lo12:x4s]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:x4s+8]
	cmp x2, x5
	bne .Lfailure

	pass
.Lfailure:
	fail
