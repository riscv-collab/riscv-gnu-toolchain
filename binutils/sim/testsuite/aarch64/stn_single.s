# mach: aarch64

# Check the store single 1-element structure to one lane instructions:
# st1, st2, st3, st4.
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
	.word 0x14131211
	.word 0x18171615
	.word 0x1c1b1a19
	.word 0x201f1e1d
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
	mov x3, #1
	mov x4, #4
	st1 {v0.b}[0], [x2], 1
	st1 {v0.b}[1], [x2], x3
	st1 {v0.h}[1], [x2], 2
	st1 {v0.s}[1], [x2], x4
	st1 {v0.d}[1], [x2]
	ldr q4, [x1]
	addv b4, v4.16b
	mov x5, v4.d[0]
	cmp x5, #136
	bne .Lfailure

	mov x2, x1
	mov x3, #16
	mov x4, #4
	st2 {v0.d, v1.d}[0], [x2], x3
	st2 {v0.s, v1.s}[2], [x2], 8
	st2 {v0.h, v1.h}[6], [x2], x4
	st2 {v0.b, v1.b}[14], [x2], 2
	st2 {v0.b, v1.b}[15], [x2]
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	mov x5, v4.d[0]
	mov x6, v5.d[0]
	cmp x5, #200
	bne .Lfailure
	cmp x6, #72
	bne .Lfailure

	mov x2, x1
	mov x3, #12
	st3 {v0.s, v1.s, v2.s}[0], [x2], 12
	st3 {v0.s, v1.s, v2.s}[1], [x2], x3
	st3 {v0.s, v1.s, v2.s}[2], [x2], 12
	st3 {v0.s, v1.s, v2.s}[3], [x2]
	mov x2, x1
	ldr q4, [x2], 16
	ldr q5, [x2], 16
	ldr q6, [x2]
	addv b4, v4.16b
	addv b5, v5.16b
	addv b6, v6.16b
	mov x4, v4.d[0]
	mov x5, v5.d[0]
	mov x6, v6.d[0]
	cmp x4, #120
	bne .Lfailure
	cmp x5, #8
	bne .Lfailure
	cmp x6, #24
	bne .Lfailure

	mov x2, x1
	mov x3, #16
	st4 {v0.s, v1.s, v2.s, v3.s}[0], [x2], 16
	st4 {v0.s, v1.s, v2.s, v3.s}[1], [x2], x3
	st4 {v0.s, v1.s, v2.s, v3.s}[2], [x2], 16
	st4 {v0.s, v1.s, v2.s, v3.s}[3], [x2]
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
	mov x5, v5.d[0]
	mov x6, v6.d[0]
	mov x7, v7.d[0]
	cmp x4, #168
	bne .Lfailure
	cmp x5, #232
	bne .Lfailure
	cmp x6, #40
	bne .Lfailure
	cmp x7, #104
	bne .Lfailure

	pass
.Lfailure:
	fail
