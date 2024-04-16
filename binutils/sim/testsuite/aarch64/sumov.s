# mach: aarch64

# Check the mov from asimd to general reg instructions: smov, umov.

.include "testutils.inc"

	.data
	.align 4
input:
	.word 0x04030201
	.word 0x08070605
	.word 0xf4f3f2f1
	.word 0xf8f7f6f5

	start
	adrp x0, input
	ldr q0, [x0, #:lo12:input]

	smov w0, v0.b[0]
	smov w3, v0.b[12]
	cmp w0, #1
	bne .Lfailure
	cmp w3, #-11
	bne .Lfailure

	smov x0, v0.b[1]
	smov x3, v0.b[13]
	cmp x0, #2
	bne .Lfailure
	cmp x3, #-10
	bne .Lfailure

	smov w0, v0.h[0]
	smov w1, v0.h[4]
	cmp w0, #0x0201
	bne .Lfailure
	cmp w1, #-3343
	bne .Lfailure

	smov x0, v0.h[1]
	smov x1, v0.h[5]
	cmp x0, #0x0403
	bne .Lfailure
	cmp x1, #-2829
	bne .Lfailure

	smov x0, v0.s[1]
	smov x1, v0.s[3]
	mov x2, #0x0605
	movk x2, #0x0807, lsl #16
	cmp x0, x2
	bne .Lfailure
	mov w3, #0xf6f5
	movk w3, #0xf8f7, lsl #16
	sxtw x3, w3
	cmp x1, x3
	bne .Lfailure

	umov w0, v0.b[0]
	umov w3, v0.b[12]
	cmp w0, #1
	bne .Lfailure
	cmp w3, #0xf5
	bne .Lfailure

	umov w0, v0.h[0]
	umov w1, v0.h[4]
	cmp w0, #0x0201
	bne .Lfailure
	mov w2, #0xf2f1
	cmp w1, w2
	bne .Lfailure

	umov w0, v0.s[0]
	umov w1, v0.s[2]
	mov w2, #0x0201
	movk w2, #0x0403, lsl #16
	cmp w0, w2
	bne .Lfailure
	mov w3, #0xf2f1
	movk w3, #0xf4f3, lsl #16
	cmp w1, w3
	bne .Lfailure

	umov x0, v0.d[0]
	adrp x1, input
	ldr x2, [x1, #:lo12:input]
	cmp x0, x2
	bne .Lfailure

	pass
.Lfailure:
	fail
