# mach: aarch64

# Check the add across vector instruction: addv.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	addv b1, v0.8b
	mov x1, v1.d[0]
	cmp x1, #36
	bne .Lfailure

	addv b1, v0.16b
	mov x1, v1.d[0]
	cmp x1, #136
	bne .Lfailure

	addv h1, v0.4h
	mov x1, v1.d[0]
	mov x2, #5136
	cmp x1, x2
	bne .Lfailure

	addv h1, v0.8h
	mov x1, v1.d[0]
	mov x2, #18496
	cmp x1, x2
	bne .Lfailure

	addv s1, v0.4s
	mov x1, v1.d[0]
	mov x2, 8220
	movk x2, 0x2824, lsl 16
	cmp x1, x2
	bne .Lfailure

	pass
.Lfailure:
	fail
