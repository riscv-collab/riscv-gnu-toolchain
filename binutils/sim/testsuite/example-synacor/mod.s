# check the MOD insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	MOD r0, 8, 3
	EQ r1, r0, 2
	JF r1, 2

	MOD r0, r0, 2
	EQ r1, r0, 0
	JF r1, 2

	pass
