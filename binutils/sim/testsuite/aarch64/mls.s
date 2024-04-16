# mach: aarch64

# Check the vector multiply subtract instruction: mls.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
m8b:
	.word 0xf1f8fd00
	.word 0xc1d0dde8
m16b:
	.word 0xf1f8fd00
	.word 0xc1d0dde8
	.word 0x71889db0
	.word 0x01203d58
m4h:
	.word 0xe7f8fc00
	.word 0x8fd0c3e8
m8h:
	.word 0xe7f8fc00
	.word 0x8fd0c3e8
	.word 0xf7884bb0
	.word 0x1f209358
m2s:
	.word 0xebf5fc00
	.word 0x5b95c3e8
m4s:
	.word 0xebf5fc00
	.word 0x5b95c3e8
	.word 0x4ad54bb0
	.word 0xb9b49358

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	movi v1.8b, #1
	mls v1.8b, v0.8b, v0.8b
	mov x1, v1.d[0]
	adrp x3, m8b
	ldr x4, [x3, #:lo12:m8b]
	cmp x1, x4
	bne .Lfailure

	movi v1.16b, #1
	mls v1.16b, v0.16b, v0.16b
	mov x1, v1.d[0]
	mov x2, v1.d[1]
	adrp x3, m16b
	ldr x4, [x3, #:lo12:m16b]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:m16b+8]
	cmp x2, x5
	bne .Lfailure

	movi v1.4h, #1
	mls v1.4h, v0.4h, v0.4h
	mov x1, v1.d[0]
	adrp x3, m4h
	ldr x4, [x3, #:lo12:m4h]
	cmp x1, x4
	bne .Lfailure

	movi v1.8h, #1
	mls v1.8h, v0.8h, v0.8h
	mov x1, v1.d[0]
	mov x2, v1.d[1]
	adrp x3, m8h
	ldr x4, [x3, #:lo12:m8h]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:m8h+8]
	cmp x2, x5
	bne .Lfailure

	movi v1.2s, #1
	mls v1.2s, v0.2s, v0.2s
	mov x1, v1.d[0]
	adrp x3, m2s
	ldr x4, [x3, #:lo12:m2s]
	cmp x1, x4
	bne .Lfailure

	movi v1.4s, #1
	mls v1.4s, v0.4s, v0.4s
	mov x1, v1.d[0]
	mov x2, v1.d[1]
	adrp x3, m4s
	ldr x4, [x3, #:lo12:m4s]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x3, #:lo12:m4s+8]
	cmp x2, x5
	bne .Lfailure

	pass
.Lfailure:
	fail
