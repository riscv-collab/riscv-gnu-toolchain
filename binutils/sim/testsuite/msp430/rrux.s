# check that rrux (synthesized as rrc with ZC bit set) works.
# mach: msp430

.include "testutils.inc"

	start

	setc		; set the carry bit to ensure ZC bit is obeyed
	mov.w	#16, r10
	rrux.w	r10
	cmp.w	#8, r10
	jeq 1f
	fail
	1: pass
