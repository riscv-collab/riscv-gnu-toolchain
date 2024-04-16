# check the CALL insn.
# mach: example

.include "testutils.inc"

	start
	CALL 3
	HALT

	POP r0
	EQ r1, r0, 2
	JF r1, 2

	pass
