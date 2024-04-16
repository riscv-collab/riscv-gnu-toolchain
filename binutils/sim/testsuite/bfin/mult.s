# Blackfin testcase for multiply
# mach: bfin

	.include "testutils.inc"

	start

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	A0 = 0;
	A1 = 0;
	R0.L = 0x0400;
	R1.L = 0x0010;
	R2.L = ( A0 = R0.L * R1.L ) (S2RND);
	R3 = 0x1 (Z);
	CC = R3 == R2;
	if CC jump 1f;
	fail
1:
	pass
