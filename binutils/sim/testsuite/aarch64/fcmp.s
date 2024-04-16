# mach: aarch64

# Check the FP compare instructions: fcmps, fcmpzs, fcmpes, fcmpzes, fcmpd,
# fcmpzd, fcmped, fcmpzed.
# For 1 operand compares, check 0, 1, -1, +Inf, -Inf.
# For 2 operand compares, check 1/1, 1/-2, -1/2, +Inf/+Inf, +Inf/-Inf.
# FIXME: Check for qNaN and sNaN when exception raising support added.

.include "testutils.inc"

	start
	fmov s0, wzr
	fcmp s0, #0.0
	bne .Lfailure
	fcmpe s0, #0.0
	bne .Lfailure
	fmov d0, xzr
	fcmp d0, #0.0
	bne .Lfailure
	fcmpe d0, #0.0
	bne .Lfailure

	fmov s0, #1.0
	fcmp s0, #0.0
	blo .Lfailure
	fcmpe s0, #0.0
	blo .Lfailure
	fmov d0, #1.0
	fcmp d0, #0.0
	blo .Lfailure
	fcmpe d0, #0.0
	blo .Lfailure

	fmov s0, #-1.0
	fcmp s0, #0.0
	bpl .Lfailure
	fcmpe s0, #0.0
	bpl .Lfailure
	fmov d0, #-1.0
	fcmp d0, #0.0
	bpl .Lfailure
	fcmpe d0, #0.0
	bpl .Lfailure

	fmov s0, #1.0
	fmov s1, wzr
	fdiv s0, s0, s1
	fcmp s0, #0.0
	blo .Lfailure
	fcmpe s0, #0.0
	blo .Lfailure
	fmov d0, #1.0
	fmov d1, xzr
	fdiv d0, d0, d1
	fcmp d0, #0.0
	blo .Lfailure
	fcmpe d0, #0.0
	blo .Lfailure

	fmov s0, #-1.0
	fmov s1, wzr
	fdiv s0, s0, s1
	fcmp s0, #0.0
	bpl .Lfailure
	fcmpe s0, #0.0
	bpl .Lfailure
	fmov d0, #-1.0
	fmov d1, xzr
	fdiv d0, d0, d1
	fcmp d0, #0.0
	bpl .Lfailure
	fcmpe d0, #0.0
	bpl .Lfailure

	fmov s0, #1.0
	fmov s1, #1.0
	fcmp s0, s1
	bne .Lfailure
	fcmpe s0, s1
	bne .Lfailure
	fmov d0, #1.0
	fmov d1, #1.0
	fcmp d0, d1
	bne .Lfailure
	fcmpe d0, d1
	bne .Lfailure

	fmov s0, #1.0
	fmov s1, #-2.0
	fcmp s0, s1
	blo .Lfailure
	fcmpe s0, s1
	blo .Lfailure
	fmov d0, #1.0
	fmov d1, #-2.0
	fcmp d0, d1
	blo .Lfailure
	fcmpe d0, d1
	blo .Lfailure

	fmov s0, #-1.0
	fmov s1, #2.0
	fcmp s0, s1
	bpl .Lfailure
	fcmpe s0, s1
	bpl .Lfailure
	fmov d0, #-1.0
	fmov d1, #2.0
	fcmp d0, d1
	bpl .Lfailure
	fcmpe d0, d1
	bpl .Lfailure

	fmov s0, #1.0
	fmov s1, wzr
	fdiv s0, s0, s1
	fcmp s0, s0
	bne .Lfailure
	fcmpe s0, s0
	bne .Lfailure
	fmov s1, #-1.0
	fmov s2, wzr
	fdiv s1, s1, s2
	fcmp s0, s1
	blo .Lfailure
	fcmpe s0, s1
	blo .Lfailure

	fmov d0, #1.0
	fmov d1, xzr
	fdiv d0, d0, d1
	fcmp d0, d0
	bne .Lfailure
	fcmpe d0, d0
	bne .Lfailure
	fmov d1, #-1.0
	fmov d2, xzr
	fdiv d1, d1, d2
	fcmp d0, d1
	blo .Lfailure
	fcmpe d0, d1
	blo .Lfailure

	pass
.Lfailure:
	fail
