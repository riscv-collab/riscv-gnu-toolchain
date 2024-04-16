# mach: aarch64

# Check the vector compare bitwise test instruction: cmtst.

.include "testutils.inc"

	.data
	.align 4
inputb:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
inputh:
	.word 0x00020001
	.word 0x00040003
	.word 0x00060005
	.word 0x00800007
inputs:
	.word 0x00000001
	.word 0x00000002
	.word 0x00000003
	.word 0x00000004
inputd:
	.word 0x00000001
	.word 0x00000000
	.word 0x00000002
	.word 0x00000000
inputd2:
	.word 0x00000003
	.word 0x00000000
	.word 0x00000004
	.word 0x00000000

	start
	adrp x0, inputb
	ldr q0, [x0, #:lo12:inputb]
	rev64 v1.16b, v0.16b

	cmtst v2.8b, v0.8b, v1.8b
	addv b3, v2.8b
	mov x1, v3.d[0]
	cmp x1, #0xfa
	bne .Lfailure

	cmtst v2.16b, v0.16b, v1.16b
	addv b3, v2.16b
	mov x1, v3.d[0]
	cmp x1, #0xf4
	bne .Lfailure

	adrp x0, inputh
	ldr q0, [x0, #:lo12:inputh]
	rev64 v1.8h, v0.8h

	cmtst v2.4h, v0.4h, v1.4h
	addv h3, v2.4h
	mov x1, v3.d[0]
	mov x2, #0xfffe
	cmp x1, x2
	bne .Lfailure

	cmtst v2.8h, v0.8h, v1.8h
	addv h3, v2.8h
	mov x1, v3.d[0]
	mov x2, #0xfffc
	cmp x1, x2
	bne .Lfailure

	adrp x0, inputs
	ldr q0, [x0, #:lo12:inputs]
	mov v1.d[0], v0.d[1]
	mov v1.d[1], v0.d[0]
	rev64 v1.4s, v1.4s

	cmtst v2.2s, v0.2s, v1.2s
	mov x1, v2.d[0]
	mov x2, #0xffffffff00000000
	cmp x1, x2
	bne .Lfailure

	cmtst v2.4s, v0.4s, v1.4s
	addv s3, v2.4s
	mov x1, v3.d[0]
	mov x2, #0xfffffffe
	cmp x1, x2
	bne .Lfailure

	adrp x0, inputd
	ldr q0, [x0, #:lo12:inputd]
	adrp x0, inputd2
	ldr q1, [x0, #:lo12:inputd2]
	
	cmtst v2.2d, v0.2d, v1.2d
	mov x1, v2.d[0]
	cmp x1, #-1
	bne .Lfailure
	mov x2, v2.d[1]
	cmp x2, #0
	bne .Lfailure

	pass
.Lfailure:
	fail
