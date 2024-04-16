# mach: aarch64

# Check the FP Conditional Select instruction: fcsel.
# Check 1/1 eq/neg, and 1/2 lt/gt.

.include "testutils.inc"

	start
	fmov s0, #1.0
	fmov s1, #1.0
	fmov s2, #-1.0
	fcmp s0, s1
	fcsel s3, s0, s2, eq
	fcmp s3, s0
	bne .Lfailure
	fcsel s3, s0, s2, ne
	fcmp s3, s2
	bne .Lfailure

	fmov s0, #1.0
	fmov s1, #2.0
	fcmp s0, s1
	fcsel s3, s0, s2, lt
	fcmp s3, s0
	bne .Lfailure
	fcsel s3, s0, s2, gt
	fcmp s3, s2
	bne .Lfailure

	fmov d0, #1.0
	fmov d1, #1.0
	fmov d2, #-1.0
	fcmp d0, d1
	fcsel d3, d0, d2, eq
	fcmp d3, d0
	bne .Lfailure
	fcsel d3, d0, d2, ne
	fcmp d3, d2
	bne .Lfailure

	fmov d0, #1.0
	fmov d1, #2.0
	fcmp d0, d1
	fcsel d3, d0, d2, lt
	fcmp d3, d0
	bne .Lfailure
	fcsel d3, d0, d2, gt
	fcmp d3, d2
	bne .Lfailure

	pass
.Lfailure:
	fail
