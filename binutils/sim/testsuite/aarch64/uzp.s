# mach: aarch64

# Check the unzip instructions: uzp1, uzp2.

.include "testutils.inc"

	.data
	.align 4
input1:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
input2:
	.word 0x14131211
	.word 0x18171615
	.word 0x1c1b1a19
	.word 0x201f1e1d
zl8b:
	.word 0x07050301
	.word 0x17151311
zu8b:
	.word 0x08060402
	.word 0x18161412
zl16b:
	.word 0x07050301
	.word 0x0f0d0b09
	.word 0x17151311
	.word 0x1f1d1b19
zu16b:
	.word 0x08060402
	.word 0x100e0c0a
	.word 0x18161412
	.word 0x201e1c1a
zl4h:
	.word 0x06050201
	.word 0x16151211
zu4h:
	.word 0x08070403
	.word 0x18171413
zl8h:
	.word 0x06050201
	.word 0x0e0d0a09
	.word 0x16151211
	.word 0x1e1d1a19
zu8h:
	.word 0x08070403
	.word 0x100f0c0b
	.word 0x18171413
	.word 0x201f1c1b
zl2s:
	.word 0x04030201
	.word 0x14131211
zu2s:
	.word 0x08070605
	.word 0x18171615
zl4s:
	.word 0x04030201
	.word 0x0c0b0a09
	.word 0x14131211
	.word 0x1c1b1a19
zu4s:
	.word 0x08070605
	.word 0x100f0e0d
	.word 0x18171615
	.word 0x201f1e1d
zl2d:
	.word 0x04030201
	.word 0x08070605
	.word 0x14131211
	.word 0x18171615
zu2d:
	.word 0x0c0b0a09
	.word 0x100f0e0d
	.word 0x1c1b1a19
	.word 0x201f1e1d

	start
	adrp x0, input1
	ldr q0, [x0, #:lo12:input1]
	adrp x0, input2
	ldr q1, [x0, #:lo12:input2]

	uzp1 v2.8b, v0.8b, v1.8b
	mov x1, v2.d[0]
	adrp x3, zl8b
	ldr x4, [x3, #:lo12:zl8b]
	cmp x1, x4
	bne .Lfailure

	uzp2 v2.8b, v0.8b, v1.8b
	mov x1, v2.d[0]
	adrp x3, zu8b
	ldr x4, [x3, #:lo12:zu8b]
	cmp x1, x4
	bne .Lfailure

	uzp1 v2.16b, v0.16b, v1.16b
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zl16b
	ldr x4, [x3, #:lo12:zl16b]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zl16b+8]
	cmp x2, x5
	bne .Lfailure

	uzp2 v2.16b, v0.16b, v1.16b
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zu16b
	ldr x4, [x3, #:lo12:zu16b]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zu16b+8]
	cmp x2, x5
	bne .Lfailure

	uzp1 v2.4h, v0.4h, v1.4h
	mov x1, v2.d[0]
	adrp x3, zl4h
	ldr x4, [x3, #:lo12:zl4h]
	cmp x1, x4
	bne .Lfailure

	uzp2 v2.4h, v0.4h, v1.4h
	mov x1, v2.d[0]
	adrp x3, zu4h
	ldr x4, [x3, #:lo12:zu4h]
	cmp x1, x4
	bne .Lfailure

	uzp1 v2.8h, v0.8h, v1.8h
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zl8h
	ldr x4, [x3, #:lo12:zl8h]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zl8h+8]
	cmp x2, x5
	bne .Lfailure

	uzp2 v2.8h, v0.8h, v1.8h
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zu8h
	ldr x4, [x3, #:lo12:zu8h]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zu8h+8]
	cmp x2, x5
	bne .Lfailure

	uzp1 v2.2s, v0.2s, v1.2s
	mov x1, v2.d[0]
	adrp x3, zl2s
	ldr x4, [x3, #:lo12:zl2s]
	cmp x1, x4
	bne .Lfailure

	uzp2 v2.2s, v0.2s, v1.2s
	mov x1, v2.d[0]
	adrp x3, zu2s
	ldr x4, [x3, #:lo12:zu2s]
	cmp x1, x4
	bne .Lfailure

	uzp1 v2.4s, v0.4s, v1.4s
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zl4s
	ldr x4, [x3, #:lo12:zl4s]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zl4s+8]
	cmp x2, x5
	bne .Lfailure

	uzp2 v2.4s, v0.4s, v1.4s
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zu4s
	ldr x4, [x3, #:lo12:zu4s]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zu4s+8]
	cmp x2, x5
	bne .Lfailure

	uzp1 v2.2d, v0.2d, v1.2d
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zl2d
	ldr x4, [x3, #:lo12:zl2d]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zl2d+8]
	cmp x2, x5
	bne .Lfailure

	uzp2 v2.2d, v0.2d, v1.2d
	mov x1, v2.d[0]
	mov x2, v2.d[1]
	adrp x3, zu2d
	ldr x4, [x3, #:lo12:zu2d]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:zu2d+8]
	cmp x2, x5
	bne .Lfailure

	pass
.Lfailure:
	fail
