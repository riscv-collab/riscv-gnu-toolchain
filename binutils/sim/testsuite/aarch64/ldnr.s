# mach: aarch64

# Check the load single 1-element structure and replicate to all lanes insns:
# ld1r, ld2r, ld3r, ld4r.
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
input2:
	.word 0x00000001
	.word 0x00000002
	.word 0x00000003
	.word 0x00000004
	.word 0x00000005
	.word 0x00000006
	.word 0x00000007
	.word 0x00000008
	.word 0x00000009
	.word 0x0000000a
	.word 0x0000000b
	.word 0x0000000c

	start
	adrp x0, input
	add x0, x0, :lo12:input
	adrp x1, input2
	add x1, x1, :lo12:input2

	mov x2, x0
	mov x3, #1
	ld1r {v0.8b}, [x2], 1
	ld1r {v1.16b}, [x2], x3
	ld1r {v2.4h}, [x2], 2
	ld1r {v3.8h}, [x2]
	addv b0, v0.8b
	addv b1, v1.16b
	addv b2, v2.8b
	addv b3, v3.16b
	mov x2, v0.d[0]
	mov x3, v1.d[0]
	mov x4, v2.d[0]
	mov x5, v3.d[0]
	cmp x2, #8
	bne .Lfailure
	cmp x3, #32
	bne .Lfailure
	cmp x4, #28
	bne .Lfailure
	cmp x5, #88
	bne .Lfailure

	mov x2, x1
	mov x3, #8
	ld2r {v0.2s, v1.2s}, [x2], 8
	ld2r {v2.4s, v3.4s}, [x2], x3
	ld2r {v4.1d, v5.1d}, [x2], 16
	ld2r {v6.2d, v7.2d}, [x2]
	addp v0.2s, v0.2s, v1.2s
	addv s2, v2.4s
	addv s3, v3.4s
	addp v4.2s, v4.2s, v5.2s
	addv s6, v6.4s
	addv s7, v7.4s
	mov w2, v0.s[0]
	mov w3, v0.s[1]
	mov x4, v2.d[0]
	mov x5, v3.d[0]
	mov w6, v4.s[0]
	mov w7, v4.s[1]
	mov x8, v6.d[0]
	mov x9, v7.d[0]
	cmp w2, #2
	bne .Lfailure
	cmp w3, #4
	bne .Lfailure
	cmp x4, #12
	bne .Lfailure
	cmp x5, #16
	bne .Lfailure
	cmp w6, #11
	bne .Lfailure
	cmp w7, #15
	bne .Lfailure
	cmp x8, #38
	bne .Lfailure
	cmp x9, #46
	bne .Lfailure

	mov x2, x0
	mov x3, #3
	ld3r {v0.8b, v1.8b, v2.8b}, [x2], 3
	ld3r {v3.8b, v4.8b, v5.8b}, [x2], x3
	ld3r {v6.8b, v7.8b, v8.8b}, [x2]
	addv b0, v0.8b
	addv b1, v1.8b
	addv b2, v2.8b
	addv b3, v3.8b
	addv b4, v4.8b
	addv b5, v5.8b
	addv b6, v6.8b
	addv b7, v7.8b
	addv b8, v8.8b
	addv b9, v9.8b
	mov x2, v0.d[0]
	mov x3, v1.d[0]
	mov x4, v2.d[0]
	mov x5, v3.d[0]
	mov x6, v4.d[0]
	mov x7, v5.d[0]
	mov x8, v6.d[0]
	mov x9, v7.d[0]
	mov x10, v8.d[0]
	cmp x2, #8
	bne .Lfailure
	cmp x3, #16
	bne .Lfailure
	cmp x4, #24
	bne .Lfailure
	cmp x5, #32
	bne .Lfailure
	cmp x6, #40
	bne .Lfailure
	cmp x7, #48
	bne .Lfailure
	cmp x8, #56
	bne .Lfailure
	cmp x9, #64
	bne .Lfailure
	cmp x10, #72
	bne .Lfailure

	mov x2, x1
	ld4r {v0.4s, v1.4s, v2.4s, v3.4s}, [x2], 16
	ld4r {v4.4s, v5.4s, v6.4s, v7.4s}, [x2]
	addv s0, v0.4s
	addv s1, v1.4s
	addv s2, v2.4s
	addv s3, v3.4s
	addv s4, v4.4s
	addv s5, v5.4s
	addv s6, v6.4s
	addv s7, v7.4s
	mov x2, v0.d[0]
	mov x3, v1.d[0]
	mov x4, v2.d[0]
	mov x5, v3.d[0]
	mov x6, v4.d[0]
	mov x7, v5.d[0]
	mov x8, v6.d[0]
	mov x9, v7.d[0]
	cmp x2, #4
	bne .Lfailure
	cmp x3, #8
	bne .Lfailure
	cmp x4, #12
	bne .Lfailure
	cmp x5, #16
	bne .Lfailure
	cmp x6, #20
	bne .Lfailure
	cmp x7, #24
	bne .Lfailure
	cmp x8, #28
	bne .Lfailure
	cmp x9, #32
	bne .Lfailure

	pass
.Lfailure:
	fail
