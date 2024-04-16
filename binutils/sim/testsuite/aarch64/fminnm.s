# mach: aarch64

# Check the FP min/max number instructions: fminnm, fmaxnm, dminnm, dmaxnm.
# For min, check 2/1, 1/0, -1/-Inf.
# For max, check 1/2, -1/0, 1/+inf.

.include "testutils.inc"

	start
	fmov s0, #2.0
	fmov s1, #1.0
	fminnm s2, s0, s1
	fcmp s2, s1
	bne .Lfailure
	fmov d0, #2.0
	fmov d1, #1.0
	fminnm d2, d0, d1
	fcmp d2, d1
	bne .Lfailure

	fmov s0, #1.0
	fmov s1, wzr
	fminnm s2, s0, s1
	fcmp s2, s1
	bne .Lfailure
	fmov d0, #1.0
	fmov d1, xzr
	fminnm d2, d0, d1
	fcmp d2, d1
	bne .Lfailure

	fmov s0, #-1.0
	fmov s1, wzr
	fdiv s1, s0, s1
	fminnm s2, s0, s1
	fcmp s2, s1
	bne .Lfailure
	fmov d0, #-1.0
	fmov d1, xzr
	fdiv d1, d0, d1
	fminnm d1, d0, d1
	fcmp d0, d0
	bne .Lfailure

	fmov s0, #1.0
	fmov s1, #2.0
	fmaxnm s2, s0, s1
	fcmp s2, s1
	bne .Lfailure
	fmov d0, #1.0
	fmov d1, #2.0
	fmaxnm d2, d0, d1
	fcmp d2, d1
	bne .Lfailure

	fmov s0, #-1.0
	fmov s1, wzr
	fmaxnm s2, s0, s1
	fcmp s2, s1
	bne .Lfailure
	fmov d0, #-1.0
	fmov d1, xzr
	fmaxnm d2, d0, d1
	fcmp d2, d1
	bne .Lfailure

	fmov s0, #1.0
	fmov s1, wzr
	fdiv s1, s0, s1
	fmaxnm s2, s0, s1
	fcmp s2, s1
	bne .Lfailure
	fmov d0, #1.0
	fmov d1, xzr
	fdiv d1, d0, d1
	fmaxnm d1, d0, d1
	fcmp d0, d0
	bne .Lfailure

	pass
.Lfailure:
	fail
