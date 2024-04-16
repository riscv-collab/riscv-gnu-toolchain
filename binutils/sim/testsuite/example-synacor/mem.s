# check the RMEM & WMEM insns.
# mach: example

.include "testutils.inc"

	start
	JMP 14
	HALT
	pass

	# Read a constant address.
	RMEM r0, 1
	EQ r1, r0, 14
	JF r1, 2

	# Change the first JMP to skip HALT and hit the pass.
	WMEM 1, 3

	# Read an address in a register.
	SET r2, 1
	RMEM r0, r2
	EQ r1, r0, 3
	JF r1, 2

	JMP 0
