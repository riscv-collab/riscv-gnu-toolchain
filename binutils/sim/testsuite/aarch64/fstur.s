# mach: aarch64

# Check the FP store unscaled offset instructions: fsturs, fsturd, fsturq.
# Check the values -1, and XXX_MAX, which tests all bits.
# Check with offsets -256 and 255, which tests all bits.
# Also tests the FP load unscaled offset instructions: fldurs, fldurd, fldurq.

.include "testutils.inc"

	.data
	.align 4
fm1:
	.word 3212836864
fmax:
	.word 2139095039
ftmp:
	.word 0

dm1:
	.word 0
	.word -1074790400
dmax:
	.word 4294967295
	.word 2146435071
dtmp:
	.word 0
	.word 0

ldm1:
	.word	0
	.word	0
	.word	0
	.word	-1073807360
ldmax:
	.word	4294967295
	.word	4294967295
	.word	4294967295
	.word	2147418111
ldtmp:
	.word 0
	.word 0
	.word 0
	.word 0

	start
	adrp x1, ftmp
	add x1, x1, :lo12:ftmp

	adrp x0, fm1
	add x0, x0, :lo12:fm1
	sub x5, x0, #255
	sub x6, x1, #255
	movi d2, #0
	ldur s2, [x5, #255]
	stur s2, [x6, #255]
	ldr w3, [x0]
	ldr w4, [x1]
	cmp w3, w4
	bne .Lfailure

	adrp x0, fmax
	add x0, x0, :lo12:fmax
	add x5, x0, #256
	add x6, x1, #256
	movi d2, #0
	ldur s2, [x5, #-256]
	stur s2, [x6, #-256]
	ldr w3, [x0]
	ldr w4, [x1]
	cmp w3, w4
	bne .Lfailure

	adrp x1, dtmp
	add x1, x1, :lo12:dtmp

	adrp x0, dm1
	add x0, x0, :lo12:dm1
	sub x5, x0, #255
	sub x6, x1, #255
	movi d2, #0
	ldur d2, [x5, #255]
	stur d2, [x6, #255]
	ldr x3, [x0]
	ldr x4, [x1]
	cmp x3, x4
	bne .Lfailure

	adrp x0, dmax
	add x0, x0, :lo12:dmax
	add x5, x0, #256
	add x6, x1, #256
	movi d2, #0
	ldur d2, [x5, #-256]
	stur d2, [x6, #-256]
	ldr x3, [x0]
	ldr x4, [x1]
	cmp x3, x4
	bne .Lfailure

	adrp x1, ldtmp
	add x1, x1, :lo12:ldtmp

	adrp x0, ldm1
	add x0, x0, :lo12:ldm1
	sub x5, x0, #255
	sub x6, x1, #255
	movi v2.2d, #0
	ldur q2, [x5, #255]
	stur q2, [x6, #255]
	ldr x3, [x0]
	ldr x4, [x1]
	cmp x3, x4
	bne .Lfailure
	ldr x3, [x0, 8]
	ldr x4, [x1, 8]
	cmp x3, x4
	bne .Lfailure

	adrp x0, ldmax
	add x0, x0, :lo12:ldmax
	add x5, x0, #256
	add x6, x1, #256
	movi v2.2d, #0
	ldur q2, [x5, #-256]
	stur q2, [x6, #-256]
	ldr x3, [x0]
	ldr x4, [x1]
	cmp x3, x4
	bne .Lfailure
	ldr x3, [x0, 8]
	ldr x4, [x1, 8]
	cmp x3, x4
	bne .Lfailure

	pass
.Lfailure:
	fail
