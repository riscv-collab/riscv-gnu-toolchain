# mach: aarch64

# Check the vector multiply add instruction: mla.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
m8b:
	.word 0x110a0502
	.word 0x4132251a
m16b:
	.word 0x110a0502
	.word 0x4132251a
	.word 0x917a6552
	.word 0x01e2c5aa
m4h:
	.word 0x180a0402
	.word 0x70323c1a
m8h:
	.word 0x180a0402
	.word 0x70323c1a
	.word 0x087ab452
	.word 0xe0e26caa
m2s:
	.word 0x140a0402
	.word 0xa46a3c1a
m4s:
	.word 0x140a0402
	.word 0xa46a3c1a
	.word 0xb52ab452
	.word 0x464b6caa

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	movi v1.8b, #1
	mla v1.8b, v0.8b, v0.8b
	mov x1, v1.d[0]
	adrp x3, m8b
	ldr x4, [x3, #:lo12:m8b]
	cmp x1, x4
	bne .Lfailure

	movi v1.16b, #1
	mla v1.16b, v0.16b, v0.16b
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
	mla v1.4h, v0.4h, v0.4h
	mov x1, v1.d[0]
	adrp x3, m4h
	ldr x4, [x3, #:lo12:m4h]
	cmp x1, x4
	bne .Lfailure

	movi v1.8h, #1
	mla v1.8h, v0.8h, v0.8h
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
	mla v1.2s, v0.2s, v0.2s
	mov x1, v1.d[0]
	adrp x3, m2s
	ldr x4, [x3, #:lo12:m2s]
	cmp x1, x4
	bne .Lfailure

	movi v1.4s, #1
	mla v1.4s, v0.4s, v0.4s
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
