# Blackfin testcase for HW Loops (LT) LSB behavior
# mach: bfin

	.include "testutils.inc"

	start

	# Loading LT should always clear LSB
	imm32 R6, 0xaaaa5555
	R4 = R6;
	BITCLR (R4, 0);

	LT0 = R6;
	LT1 = R6;

	R0 = LT0;
	CC = R0 == R4;
	IF ! CC JUMP 1f;

	R0 = LT1;
	CC = R0 == R4;
	IF ! CC JUMP 1f;

	pass
1:	fail
