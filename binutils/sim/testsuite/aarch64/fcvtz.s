# mach: aarch64

# Check the FP convert to int round toward zero instructions: fcvtszs32,
# fcvtszs, fcvtszd32, fcvtszd, fcvtzu.
# For 32-bit signed convert, test values -1.5, INT_MAX, and INT_MIN.
# For 64-bit signed convert, test values -1.5, LONG_MAX, and LONG_MIN.
# For 32-bit unsigned convert, test values 1.5, INT_MAX, and UINT_MAX.
# For 64-bit unsigned convert, test values 1.5, LONG_MAX, and ULONG_MAX.

	.data
	.align 4
fm1p5:
	.word	3217031168
fimax:
	.word	1325400064
fimin:
	.word	3472883712
flmax:
	.word	1593835520
flmin:
	.word	3741319168
f1p5:
	.word	1069547520
fuimax:
	.word	1333788672
fulmax:
	.word	1602224128

dm1p5:
	.word	0
	.word	-1074266112
dimax:
	.word	4290772992
	.word	1105199103
dimin:
	.word	0
	.word	-1042284544
dlmax:
	.word	0
	.word	1138753536
dlmin:
	.word	0
	.word	-1008730112
d1p5:
	.word	0
	.word	1073217536
duimax:
	.word	4292870144
	.word	1106247679
dulmax:
	.word	0
	.word	1139802112

.include "testutils.inc"

	start
	adrp x0, fm1p5
	ldr s0, [x0, #:lo12:fm1p5]
	fcvtzs w1, s0
	cmp w1, #-1
	bne .Lfailure
	adrp x0, fimax
	ldr s0, [x0, #:lo12:fimax]
	fcvtzs w1, s0
	mov w2, #0x7fffffff
	cmp w1, w2
	bne .Lfailure
	adrp x0, fimin
	ldr s0, [x0, #:lo12:fimin]
	fcvtzs w1, s0
	mov w2, #0x80000000
	cmp w1, w2
	bne .Lfailure

	adrp x0, fm1p5
	ldr s0, [x0, #:lo12:fm1p5]
	fcvtzs x1, s0
	cmp x1, #-1
	bne .Lfailure
	adrp x0, flmax
	ldr s0, [x0, #:lo12:flmax]
	fcvtzs x1, s0
	mov x2, #0x7fffffffffffffff
	cmp x1, x2
	bne .Lfailure
	adrp x0, flmin
	ldr s0, [x0, #:lo12:flmin]
	fcvtzs x1, s0
	mov x2, #0x8000000000000000
	cmp x1, x2
	bne .Lfailure

	adrp x0, dm1p5
	ldr d0, [x0, #:lo12:dm1p5]
	fcvtzs w1, d0
	cmp w1, #-1
	bne .Lfailure
	adrp x0, dimax
	ldr d0, [x0, #:lo12:dimax]
	fcvtzs w1, d0
	mov w2, #0x7fffffff
	cmp w1, w2
	bne .Lfailure
	adrp x0, dimin
	ldr d0, [x0, #:lo12:dimin]
	fcvtzs w1, d0
	mov w2, #0x80000000
	cmp w1, w2
	bne .Lfailure

	adrp x0, dm1p5
	ldr d0, [x0, #:lo12:dm1p5]
	fcvtzs x1, d0
	cmp x1, #-1
	bne .Lfailure
	adrp x0, dlmax
	ldr d0, [x0, #:lo12:dlmax]
	fcvtzs x1, d0
	mov x2, #0x7fffffffffffffff
	cmp x1, x2
	bne .Lfailure
	adrp x0, dlmin
	ldr d0, [x0, #:lo12:dlmin]
	fcvtzs x1, d0
	mov x2, #0x8000000000000000
	cmp x1, x2
	bne .Lfailure

	adrp x0, f1p5
	ldr s0, [x0, #:lo12:f1p5]
	fcvtzu w1, s0
	cmp w1, #1
	bne .Lfailure
	adrp x0, fimax
	ldr s0, [x0, #:lo12:fimax]
	fcvtzu w1, s0
	mov w2, #0x80000000
	cmp w1, w2
	bne .Lfailure
	adrp x0, fuimax
	ldr s0, [x0, #:lo12:fuimax]
	fcvtzu w1, s0
	mov w2, #0xffffffff
	cmp w1, w2
	bne .Lfailure

	adrp x0, f1p5
	ldr s0, [x0, #:lo12:f1p5]
	fcvtzu x1, s0
	cmp x1, #1
	bne .Lfailure
	adrp x0, flmax
	ldr s0, [x0, #:lo12:flmax]
	fcvtzu x1, s0
	mov x2, #0x8000000000000000
	cmp x1, x2
	bne .Lfailure
	adrp x0, fulmax
	ldr s0, [x0, #:lo12:fulmax]
	fcvtzu x1, s0
	mov x2, #0xffffffffffffffff
	cmp x1, x2
	bne .Lfailure

	adrp x0, d1p5
	ldr d0, [x0, #:lo12:d1p5]
	fcvtzu w1, d0
	cmp w1, #1
	bne .Lfailure
	adrp x0, dimax
	ldr d0, [x0, #:lo12:dimax]
	fcvtzu w1, d0
	mov w2, #0x7fffffff
	cmp w1, w2
	bne .Lfailure
	adrp x0, duimax
	ldr d0, [x0, #:lo12:duimax]
	fcvtzu w1, d0
	mov w2, #0xffffffff
	cmp w1, w2
	bne .Lfailure

	adrp x0, d1p5
	ldr d0, [x0, #:lo12:d1p5]
	fcvtzu x1, d0
	cmp x1, #1
	bne .Lfailure
	adrp x0, dlmax
	ldr d0, [x0, #:lo12:dlmax]
	fcvtzu x1, d0
	mov x2, #0x8000000000000000
	cmp x1, x2
	bne .Lfailure
	adrp x0, dulmax
	ldr d0, [x0, #:lo12:dulmax]
	fcvtzu x1, d0
	mov x2, #0xffffffffffffffff
	cmp x1, x2
	bne .Lfailure

	pass
.Lfailure:
	fail
