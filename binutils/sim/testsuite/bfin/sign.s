# Blackfin testcase for signbits
# mach: bfin

	.include "testutils.inc"

	start

	.macro check_alu_signbits areg:req
	\areg = 0;
	R0 = 0x10 (Z);
	\areg\().x = R0;

	imm32 r0, 0x60038;

	R0.L = SIGNBITS \areg;

	imm32 r1, 0x6fffa;
	CC = R1 == R0;
	if ! CC jump 1f;
	.endm

	check_alu_signbits A0
	check_alu_signbits A1

	pass
1:
	fail
