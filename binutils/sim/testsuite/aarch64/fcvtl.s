# mach: aarch64

# Check the FP convert to longer precision: fcvtl, fcvtl2.
# Test values 1.5, -1.5, INTMAX, and INT_MIN.

.include "testutils.inc"

	.data
	.align 4
input:
	.word	1069547520
	.word	3217031168
	.word	1325400064
	.word	3472883712
d1p5:
	.word	0
	.word	1073217536
dm1p5:
	.word	0
	.word	-1074266112
dimax:
	.word	0
	.word	1105199104
dimin:
	.word	0
	.word	-1042284544

	start
	adrp x0, input
	add x0, x0, #:lo12:input
	ld1 {v0.4s}, [x0]

	fcvtl v1.2d, v0.2s
	mov x1, v1.d[0]
	adrp x2, d1p5
	ldr x3, [x2, #:lo12:d1p5]
	cmp x1, x3
	bne .Lfailure
	mov x1, v1.d[1]
	adrp x2, dm1p5
	ldr x3, [x2, #:lo12:dm1p5]
	cmp x1, x3
	bne .Lfailure

	fcvtl2 v2.2d, v0.4s
	mov x1, v2.d[0]
	adrp x2, dimax
	ldr x3, [x2, #:lo12:dimax]
	cmp x1, x3
	bne .Lfailure
	mov x1, v2.d[1]
	adrp x2, dimin
	ldr x3, [x2, #:lo12:dimin]
	cmp x1, x3
	bne .Lfailure

	pass
.Lfailure:
	fail
