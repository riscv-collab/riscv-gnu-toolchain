# Blackfin testcase for parallel VIT_MAX (taken from PRM)
# mach: bfin

	.include "testutils.inc"

	start

	loadsym P0, scratch

	# Do parallel VIT_MAX's with stores to same reg; don't really
	# care what the result is of VIT_MAX as long as it doesn't
	# clobber the memory store.

	imm32 R1, 0xFFFF0000
	imm32 R2, 0x0000FFFF
	imm32 R0, 0xFACE
	R0 = VIT_MAX (R1, R2) (ASL) || W[P0] = R0.L;
	imm32 R0, 0xFACE
	R4 = W[P0];
	CC = R4 == R0;
	IF !CC JUMP 1f;

	imm32 R5, 0xFEEDBEEF
	imm32 R4, 0xDEAF0000
	imm32 R6, 0xFACE
	R6 = VIT_MAX (R5, R4) (ASR) || W[P0] = R6.L;
	imm32 R6, 0xFACE
	R4 = W[P0];
	CC = R4 == R6;
	IF !CC JUMP 1f;

	imm32 R3, 0xFFFF0000
	imm32 R1, 0xFACE
	R1.L = VIT_MAX (R3) (ASL) || W[P0] = R1.L;
	imm32 R1, 0xFACE
	R4 = W[P0];
	CC = R4 == R1;
	IF !CC JUMP 1f;

	imm32 R2, 0x1234FADE
	imm32 R5, 0xFACE
	R5.L = VIT_MAX (R2) (ASR) || W[P0] = R5.L;
	imm32 R5, 0xFACE
	R4 = W[P0];
	CC = R4 == R5;
	IF !CC JUMP 1f;

	pass
1:	fail

	.data
scratch:
	.dw 0xffff
