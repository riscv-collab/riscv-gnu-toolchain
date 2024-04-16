# check the RET insn.
# mach: example

.include "testutils.inc"

	start
	JMP 13
	pass

	SET r5, 2
	PUSH r5
	RET
	HALT
