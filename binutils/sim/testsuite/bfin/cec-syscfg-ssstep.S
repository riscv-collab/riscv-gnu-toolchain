# Blackfin testcase for hardware single stepping
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	start

	# Set up exception handler
	imm32 P4, EVT3;
	loadsym R1, _evx;
	[P4] = R1;

	# Enable single stepping
	R0 = 1;
	SYSCFG = R0;

	# Lower to the code we want to single step through
	R1 = 1;
	imm32 R5, 0xffff
	R6 = 0;
	R7 = 0;
	loadsym R1, _usr;
	RETI = R1;
	RTI;

_usr:
	# Single step and set a new bit every time
	BITSET (R7, 0);
	BITSET (R7, 1);
	BITSET (R7, 2);
	BITSET (R7, 3);
	BITSET (R7, 4);
	BITSET (R7, 5);
	BITSET (R7, 6);
	BITSET (R7, 7);
	BITSET (R7, 8);
	BITSET (R7, 9);
	BITSET (R7, 10);
	BITSET (R7, 11);
	BITSET (R7, 12);
	BITSET (R7, 13);
	BITSET (R7, 14);
	BITSET (R7, 15);
	JUMP fail_lvl;

_evx:
	# Make sure exception reason is single step
	R3 = SEQSTAT;
	R4 = 0x3f;
	R3 = R3 & R4;
	R4 = 0x10;
	CC = R3 == R4;
	IF !CC JUMP fail_lvl;

	# Set a new bit in R6 every single step to match R7
	CC = R1;
	R6 = ROT R6 BY 1;
	CC = R6 == R7;
	IF !CC JUMP fail_lvl;

	# Do it through each bit
	CC = R5 == R6;
	IF CC JUMP pass_lvl;

	RTX;

pass_lvl:
	dbg_pass;
fail_lvl:
	dbg_fail;
