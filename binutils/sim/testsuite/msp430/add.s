# check that basic add insn works.
# mach: msp430

.include "testutils.inc"

	start

	mov #10, r4
	add #23, r4
	cmp #33, r4
	jne 1f

	cmp #32, r4
	jlo 1f

	cmp #34, r4
	jhs 1f

	pass
1:	fail
