# Blackfin testcase for signbits
# mach: bfin

	.include "testutils.inc"

	start

xx:
	R0 = 1;
	CALL red;
	JUMP.L aa;

	.align 16
aa:
	R0 = 2;
	CALL red;
	JUMP.S bb;

	.align 16
bb:
	R0 = 3;
	CALL red;
	JUMP.S ccd;

	.align 16
red:
	RTS;

	.align 16
ccd:
	R1 = 3 (Z);
	CC = R0 == R1
	if CC jump 1f;
	fail
1:
	pass
