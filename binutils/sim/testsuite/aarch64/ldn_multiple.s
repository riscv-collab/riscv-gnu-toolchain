# mach: aarch64

# Check the load multiple structure instructions: ld1, ld2, ld3, ld4.
# Check the addressing modes: no offset, post-index immediate offset,
# post-index register offset.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0x0c0b0a09
	.word 0x100f0e0d
	.word 0xfcfdfeff
	.word 0xf8f9fafb
	.word 0xf4f5f6f7
	.word 0xf0f1f2f3

	start
	adrp x0, input
	add x0, x0, :lo12:input

	mov x2, x0
	mov x3, #16
	ld1 {v0.16b}, [x2], 16
	ld1 {v1.8h}, [x2], x3
	addv b4, v0.16b
	addv b5, v1.16b
	mov x4, v4.d[0]
	cmp x4, #136
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #120
	bne .Lfailure

	mov x2, x0
	mov x3, #16
	ld2 {v0.8b, v1.8b}, [x2], x3
	ld2 {v2.4h, v3.4h}, [x2], 16
	addv b4, v0.8b
	addv b5, v1.8b
	addv b6, v2.8b
	addv b7, v3.8b
	mov x4, v4.d[0]
	cmp x4, #64
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #72
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #196
	bne .Lfailure
	mov x7, v7.d[0]
	cmp x7, #180
	bne .Lfailure

	mov x2, x0
	ld3 {v0.2s, v1.2s, v2.2s}, [x2]
	addv b4, v0.8b
	addv b5, v1.8b
	addv b6, v2.8b
	mov x4, v4.d[0]
	cmp x4, #68
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #16
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #16
	bne .Lfailure

	mov x2, x0
	ld4 {v0.4h, v1.4h, v2.4h, v3.4h}, [x2]
	addv b4, v0.8b
	addv b5, v1.8b
	addv b6, v2.8b
	addv b7, v3.8b
	mov x4, v4.d[0]
	cmp x4, #0
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #0
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #0
	bne .Lfailure
	mov x7, v7.d[0]
	cmp x7, #0
	bne .Lfailure

	mov x2, x0
	ld1 {v0.4s, v1.4s}, [x2]
	addv b4, v0.16b
	addv b5, v1.16b
	mov x4, v4.d[0]
	cmp x4, #136
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #120
	bne .Lfailure

	mov x2, x0
	ld1 {v0.1d, v1.1d, v2.1d}, [x2]
	addv b4, v0.8b
	addv b5, v1.8b
	addv b6, v2.8b
	mov x4, v4.d[0]
	cmp x4, #36
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #100
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #220
	bne .Lfailure

	mov x2, x0
	ld1 {v0.1d, v1.1d, v2.1d, v3.1d}, [x2]
	addv b4, v0.8b
	addv b5, v1.8b
	addv b6, v2.8b
	mov x4, v4.d[0]
	cmp x4, #36
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #100
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #220
	bne .Lfailure

	pass
.Lfailure:
	fail
