# Blackfin testcase for VIT_MAX (taken from PRM)
# mach: bfin

	.include "testutils.inc"

	start

	imm32 R3, 0xFFFF0000
	imm32 R2, 0x0000FFFF
	A0 = 0;
	R5 = VIT_MAX (R3, R2) (ASL);
	R4 = 0 (x);
	CC = R5 == R4;
	IF !CC JUMP 1f;
	imm32 R6, 0x00000002
	R4 = A0;
	CC = R4 == R6;
	IF !CC JUMP 1f;

	imm32 R1, 0xFEEDBEEF
	imm32 R0, 0xDEAF0000
	A0 = 0;
	R7 = VIT_MAX (R1, R0) (ASR);
	imm32 R4, 0xFEED0000
	CC = R4 == R7;
	IF !CC JUMP 1f;
	imm32 R6, 0x80000000
	R2 = A0.W;
	CC = R2 == R6;
	IF !CC JUMP 1f;

	imm32 R1, 0xFFFF0000
	A0 = 0;
	R3.L = VIT_MAX (R1) (ASL);
	R3 = R3.L;
	R4 = 0 (x);
	CC = R3 == R4;
	IF !CC JUMP 1f;
	R6 = A0.W;
	CC = R6 == R4;
	IF !CC JUMP 1f;

	imm32 R1, 0x1234FADE
	imm32 R2, 0xFFFFFFFF
	A0.W = R2;
	R3.L = VIT_MAX (R1) (ASR);
	R3 = R3.L;
	imm32 R4 0x00001234
	CC = R4 == R3;
	IF !CC JUMP 1f;
	imm32 R7, 0xFFFFFFFF
	R0 = A0.W;
	CC = R7 == R0;
	IF !CC JUMP 1f;

	pass
1:	fail
