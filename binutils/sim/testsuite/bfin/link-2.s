# Blackfin testcase for link/unlink instructions
# mach: bfin

	.include "testutils.inc"

	start

	/* Make sure size arg to LINK works */
	R0 = SP;
	LINK 0x20;
	R1 = SP;
	R1 += 0x8 + 0x20;
	CC = R1 == R0;
	IF !CC JUMP 1f;

	/* Make sure UNLINK restores old SP */
	UNLINK
	R1 = SP;
	CC = R1 == R0;
	IF !CC JUMP 1f;

	pass
1:
	fail
