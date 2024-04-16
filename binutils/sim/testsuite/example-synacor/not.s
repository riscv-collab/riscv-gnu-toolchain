# check the NOT insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	SET r2, 0xc
	NOT r0, r2
	EQ r3, r0, 0x7ff3
	JF r3, 2

	pass
