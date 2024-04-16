# check the JMP insn.
# mach: example

.include "testutils.inc"

	start
	JMP 3
	HALT
	pass
