# Blackfin testcase for compare instructions
# mach: bfin

	.include "testutils.inc"

	start

	R0 = 0 (X);
	R1 = 0 (X);
	CC = R0 == R1;
	IF !CC JUMP 1f;
	IF !CC JUMP 1f (bp);
	pass
1:
	fail
