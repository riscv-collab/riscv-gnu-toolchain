# check the PUSH & POP insns.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	PUSH 1
	SET r0, 3
	PUSH r0
	POP r1
	POP r2
	EQ r7, r0, 3
	JF r7, 2
	EQ r7, r1, 3
	JF r7, 2
	EQ r7, r2, 1
	JF r7, 2

	pass
