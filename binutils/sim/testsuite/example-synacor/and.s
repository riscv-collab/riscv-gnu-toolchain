# check the AND insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	AND r2, 0xfff, 0x7f0c
	EQ r3, r2, 0xf0c
	JF r3, 2

	AND r2, r2, 0xf
	EQ r3, r2, 0xc
	JF r3, 2

	pass
