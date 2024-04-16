# check the ADD insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT

	SET r2, 2
	ADD r2, r2, r2
	EQ r3, r2, 4
	JF r3, 2

	ADD r1, 100, r2
	EQ r4, r1, 104
	JF r4, 2

	# 0x7ffe == -2
	ADD r0, r1, 0x7ffe
	EQ r4, r0, 102
	JF r4, 2

	pass
