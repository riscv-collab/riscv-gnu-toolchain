# Blackfin testcase for playing with CYCLES
# mach: bfin

	.include "testutils.inc"

	start

	R0 = 0;
	R1 = 1;
	CYCLES = R0;
	CYCLES2 = R1;

	/* CYCLES should be "small" while CYCLES2 should be R1 still */
	R2 = CYCLES;
	CC = R2 <= 3;
	if ! CC jump 1f;

	R3 = CYCLES2;
	CC = R3 == 1;
	if ! CC jump 1f;

	nop;
	mnop;
	nop;
	mnop;

	/* Test the "shadowed" CYCLES2 -- only a read of CYCLES reloads it */
	imm32 R1, 0x12345678
	CYCLES2 = R1;
	R2 = CYCLES2;
	CC = R2 == R3;
	if ! CC jump 1f;

	R2 = CYCLES;
	R2 = CYCLES2;
	CC = R2 == R1;
	if ! CC jump 1f;

	pass
1:
	fail
