# Blackfin testcase for signbits
# mach: bfin

	.include "testutils.inc"

	start

	L2 = 0;
	M2 = -4 (X);
	I2.H = 0x9000;
	I2.L = 0;
	I2 += M2 (BREV);
	R2 = I2;
	imm32 r0, 0x10000002
	CC = R2 == R0
	if CC jump 1f;

	fail
1:
	pass
