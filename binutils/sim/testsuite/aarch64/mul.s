# mach: aarch64

# Check the non-widening multiply vector instruction: mul.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
m8b:
	.word 0x10090401
	.word 0x40312419
m16b:
	.word 0x10090401
	.word 0x40312419
	.word 0x90796451
	.word 0x00e1c4a9
m4h:
	.word 0x18090401
	.word 0x70313c19
m8h:
	.word 0x18090401
	.word 0x70313c19
	.word 0x0879b451
	.word 0xe0e16ca9
m2s:
	.word 0x140a0401
	.word 0xa46a3c19
m4s:
	.word 0x140a0401
	.word 0xa46a3c19
	.word 0xb52ab451
	.word 0x464b6ca9

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	mul v1.8b, v0.8b, v0.8b
	mov x1, v1.d[0]
	adrp x3, m8b
	ldr x4, [x0, #:lo12:m8b]
	cmp x1, x4
	bne .Lfailure

	mul v1.16b, v0.16b, v0.16b
	mov x1, v1.d[0]
	mov x2, v1.d[1]
	adrp x3, m16b
	ldr x4, [x0, #:lo12:m16b]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x0, #:lo12:m16b+8]
	cmp x2, x5
	bne .Lfailure

	mul v1.4h, v0.4h, v0.4h
	mov x1, v1.d[0]
	adrp x3, m4h
	ldr x4, [x0, #:lo12:m4h]
	cmp x1, x4
	bne .Lfailure

	mul v1.8h, v0.8h, v0.8h
	mov x1, v1.d[0]
	mov x2, v1.d[1]
	adrp x3, m8h
	ldr x4, [x0, #:lo12:m8h]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x0, #:lo12:m8h+8]
	cmp x2, x5
	bne .Lfailure

	mul v1.2s, v0.2s, v0.2s
	mov x1, v1.d[0]
	adrp x3, m2s
	ldr x4, [x0, #:lo12:m2s]
	cmp x1, x4
	bne .Lfailure

	mul v1.4s, v0.4s, v0.4s
	mov x1, v1.d[0]
	mov x2, v1.d[1]
	adrp x3, m4s
	ldr x4, [x0, #:lo12:m4s]
	cmp x1, x4
	bne .Lfailure
	ldr x5, [x0, #:lo12:m4s+8]
	cmp x2, x5
	bne .Lfailure

	pass
.Lfailure:
	fail
