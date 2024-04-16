# Blackfin testcase for circular buffer limits
# mach: bfin

	.include "testutils.inc"

	start

	B0 = 0 (X);
	I0 = 0x1100 (X);
	L0 = 0x10c0 (X);
	M0 = 0 (X);
	I0 += M0;
	R0 = I0;

	R1 = 0x40 (Z);
	CC = R1 == R0
	if CC jump 1f;
	fail
1:
	pass
