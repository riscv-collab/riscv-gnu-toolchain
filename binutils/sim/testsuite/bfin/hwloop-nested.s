# Blackfin testcase for overlapping nested hwloops (LB)
# mach: bfin

	.include "testutils.inc"

	start

	R0 = 0;
	R1 = 0;
	P0 = 2;
	P1 = 2;
	LSETUP (1f, 3f) LC0 = P0;
1:	R0 += 1;

	LSETUP (2f, 3f) LC1 = P1;
2:	R1 += 1;

	CC = R1 == 2;
	IF !CC JUMP 3f;
	CC = R0 == 1;
	IF !CC JUMP fail;
	R3 = LC0;
	CC = R3 == 2;
	IF !CC JUMP fail;
	R3 = LC1;
	CC = R3 == 1;
	IF !CC JUMP fail;
	pass

3:	nop;

fail:
	fail
