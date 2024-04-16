# Blackfin testcase for USP handling
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	start

	imm32 R5, 0x44455566
	imm32 R6, 0x12345678
	imm32 R7, 0x9abcdef0

	imm32 p0, EVT3;
	loadsym r0, exception;
	[p0] = r0;

	loadsym r0, usermode;
	reti = r0;

	SP = R6;
	USP = R7;
	RTI;

usermode:
	# SP should now be USP
	R1 = SP;
	CC = R1 == R7;
	IF !CC JUMP fail;

	# Now set SP to another value
	SP = R5;

	# Move up to exception space
	EXCPT 0;

exception:
	# SP should be the same as original, but USP should change
	R1 = SP;
	CC = R1 == R6;
	IF !CC JUMP fail;

	R1 = USP;
	CC = R1 == R5;
	IF !CC JUMP fail;

	dbg_pass

fail:
	dbg_fail
