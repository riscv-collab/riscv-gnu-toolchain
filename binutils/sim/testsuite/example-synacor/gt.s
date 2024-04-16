# check the GT insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	GT r0, 3, 2
	EQ r1, r0, 1
	JF r1, 2

	GT r0, 2, 2
	EQ r1, r0, 0
	JF r1, 2

	GT r0, 1, 2
	EQ r1, r0, 0
	JF r1, 2

	SET r2, 3
	SET r3, 4
	GT r0, r2, r3
	EQ r1, r0, 0
	JF r1, 2
	GT r0, r3, r2
	EQ r1, r0, 1
	JF r1, 2

	pass
