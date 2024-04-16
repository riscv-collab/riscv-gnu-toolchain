# check the MULT insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	MULT r0, 3, 2
	EQ r1, r0, 6
	JF r1, 2

	MULT r0, r0, 8
	EQ r1, r0, 48
	JF r1, 2

	pass
