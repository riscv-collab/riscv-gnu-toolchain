# mach: aarch64

# Check the store multiple structure instructions: st1, st2, st3, st4.
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
output:
	.zero 64

	start
	adrp x0, input
	add x0, x0, :lo12:input
	adrp x1, output
	add x1, x1, :lo12:output

	mov x2, x0
	ldr q0, [x2], 16
	ldr q1, [x2]
	mov x2, x0
	ldr q2, [x2], 16
	ldr q3, [x2]

	mov x2, x1
	mov x3, #16
	st1 {v0.16b}, [x2], 16
	st1 {v1.8h}, [x2], x3
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	mov x4, v4.d[0]
	cmp x4, #136
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #120
	bne .Lfailure

	mov x2, x1
	mov x3, #16
	st2 {v0.8b, v1.8b}, [x2], 16
	st2 {v2.4h, v3.4h}, [x2], x3
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	mov x4, v4.d[0]
	cmp x4, #0
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #0
	bne .Lfailure

	mov x2, x1
	st3 {v0.4s, v1.4s, v2.4s}, [x2]
	ldr q4, [x2], 16
	ldr q5, [x2], 16
	ldr q6, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	addv b6, v6.16b
	mov x4, v4.d[0]
	cmp x4, #36
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #0
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #100
	bne .Lfailure

	mov x2, x1
	st4 {v0.2d, v1.2d, v2.2d, v3.2d}, [x2]
	ldr q4, [x2], 16
	ldr q5, [x2], 16
	ldr q6, [x2], 16
	ldr q7, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	addv b6, v6.16b
	addv b7, v7.16b
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

	pass

	mov x2, x1
	st1 {v0.2s, v1.2s}, [x2], 16
	st1 {v2.1d, v3.1d}, [x2]
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	mov x4, v4.d[0]
	cmp x4, #0
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #0
	bne .Lfailure

	mov x2, x1
	st1 {v0.2d, v1.2d, v2.2d}, [x2]
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2], 16
	ldr q6, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	addv b6, v6.16b
	mov x4, v4.d[0]
	cmp x4, #136
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #120
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #136
	bne .Lfailure

	mov x2, x1
	st1 {v0.2d, v1.2d, v2.2d, v3.2d}, [x2]
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2], 16
	ldr q6, [x2], 16
	ldr q7, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	addv b6, v6.16b
	addv b7, v7.16b
	mov x4, v4.d[0]
	cmp x4, #136
	bne .Lfailure
	mov x5, v5.d[0]
	cmp x5, #120
	bne .Lfailure
	mov x6, v6.d[0]
	cmp x6, #136
	bne .Lfailure
	mov x7, v7.d[0]
	cmp x7, #120
	bne .Lfailure

	pass
.Lfailure:
	fail
