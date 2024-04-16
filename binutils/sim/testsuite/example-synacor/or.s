# check the OR insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	OR r2, 0xf, 0x80
	EQ r3, r2, 0x8f
	JF r3, 2

	OR r2, r2, 0xff
	EQ r3, r2, 0xff
	JF r3, 2

	pass
