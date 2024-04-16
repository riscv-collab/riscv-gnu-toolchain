# check the SET insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	SET r2, 2
	EQ r3, r2, 2
	JF r3, 2
	SET r1, 1
	EQ r3, r1, 1
	JF r3, 2
	SET r0, r2
	EQ r3, r0, 2
	JF r3, 2

	pass
