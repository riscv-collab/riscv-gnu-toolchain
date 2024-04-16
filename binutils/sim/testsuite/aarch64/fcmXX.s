# mach: aarch64

# Check the FP scalar compare zero instructions: fcmeq, fcmle, fcmlt, fcmge,
# fcmgt.
# Check values -1, 0, and 1.

.include "testutils.inc"

	start
	fmov s0, wzr
	fcmeq s1, s0, #0.0
	mov w0, v1.s[0]
	cmp w0, #-1
	bne .Lfailure
	fmov s0, #-1.0
	fcmeq s1, s0, #0.0
	mov w0, v1.s[0]
	cmp w0, #0
	bne .Lfailure
	fmov d0, xzr
	fcmeq d1, d0, #0.0
	mov x0, v1.d[0]
	cmp x0, #-1
	bne .Lfailure
	fmov d0, #1.0
	fcmeq d1, d0, #0.0
	mov x0, v1.d[0]
	cmp x0, #0
	bne .Lfailure

	fmov s0, #-1.0
	fcmle s1, s0, #0.0
	mov w0, v1.s[0]
	cmp w0, #-1
	bne .Lfailure
	fmov d0, #-1.0
	fcmle d1, d0, #0.0
	mov x0, v1.d[0]
	cmp x0, #-1
	bne .Lfailure

	fmov s0, #-1.0
	fcmlt s1, s0, #0.0
	mov w0, v1.s[0]
	cmp w0, #-1
	bne .Lfailure
	fmov d0, #-1.0
	fcmlt d1, d0, #0.0
	mov x0, v1.d[0]
	cmp x0, #-1
	bne .Lfailure

	fmov s0, #1.0
	fcmge s1, s0, #0.0
	mov w0, v1.s[0]
	cmp w0, #-1
	bne .Lfailure
	fmov d0, #1.0
	fcmge d1, d0, #0.0
	mov x0, v1.d[0]
	cmp x0, #-1
	bne .Lfailure

	fmov s0, #1.0
	fcmgt s1, s0, #0.0
	mov w0, v1.s[0]
	cmp w0, #-1
	bne .Lfailure
	fmov d0, #1.0
	fcmgt d1, d0, #0.0
	mov x0, v1.d[0]
	cmp x0, #-1
	bne .Lfailure

	pass
.Lfailure:
	fail
