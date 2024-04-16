#mach: aarch64

# Check the extend long instructions: sxtl, sxtl2, uxtl, uxtl2.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0xfcfdfeff
	.word 0xf8f9fafb

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	uxtl v1.8h, v0.8b
	uxtl2 v2.8h, v0.16b
	addv h3, v1.8h
	addv h4, v2.8h
	mov x1, v3.d[0]
	mov x2, v4.d[0]
	cmp x1, #36
	bne .Lfailure
	cmp x2, #2012
	bne .Lfailure

	uxtl v1.4s, v0.4h
	uxtl2 v2.4s, v0.8h
	addv s3, v1.4s
	addv s4, v2.4s
	mov x1, v3.d[0]
	mov x2, v4.d[0]
	mov x3, #5136
	cmp x1, x3
	bne .Lfailure
	mov x4, #0xeff0
	movk x4, 0x3, lsl #16
	cmp x2, x4
	bne .Lfailure

	uxtl v1.2d, v0.2s
	uxtl2 v2.2d, v0.4s
	addv s3, v1.4s
	addv s4, v2.4s
	mov x1, v3.d[0]
	mov x2, v4.d[0]
	mov x3, #0x0806
	movk x3, #0x0c0a, lsl #16
	cmp x1, x3
	bne .Lfailure
	mov x4, #0xf9fa
	movk x4, #0xf5f7, lsl #16
	cmp x2, x4
	bne .Lfailure

	sxtl v1.8h, v0.8b
	sxtl2 v2.8h, v0.16b
	addv h3, v1.8h
	addv h4, v2.8h
	mov x1, v3.d[0]
	mov x2, v4.d[0]
	cmp x1, #36
	bne .Lfailure
	mov x3, #0xffdc
	cmp x2, x3
	bne .Lfailure

	sxtl v1.4s, v0.4h
	sxtl2 v2.4s, v0.8h
	addv s3, v1.4s
	addv s4, v2.4s
	mov x1, v3.d[0]
	mov x2, v4.d[0]
	mov x3, #5136
	cmp x1, x3
	bne .Lfailure
	mov x4, #0xeff0
	movk x4, 0xffff, lsl #16
	bne .Lfailure

	sxtl v1.2d, v0.2s
	sxtl2 v2.2d, v0.4s
	addv s3, v1.4s
	addv s4, v2.4s
	mov x1, v3.d[0]
	mov x2, v4.d[0]
	mov x3, #0x0806
	movk x3, #0x0c0a, lsl #16
	cmp x1, x3
	bne .Lfailure
	mov x4, #0xf9f8
	movk x4, #0xf5f7, lsl #16
	cmp x2, x4
	bne .Lfailure

	pass
.Lfailure:
	fail
