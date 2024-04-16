# Blackfin testcase for link/unlink instructions
# mach: bfin

	.include "testutils.inc"

	start

	/* give FP/RETS known/different values */
	R7.H = 0xdead;
	R7.L = 0x1234;
	RETS = R7;
	R6 = R7;
	R6 += 0x23;
	FP = R6;

	/* SP should have moved by -8 bytes (to push FP/RETS) */
	R0 = SP;
	LINK 0;
	R1 = SP;
	R1 += 8;
	CC = R0 == R1;
	IF !CC JUMP 1f;

	/* FP should now have the same value as SP */
	R1 = SP;
	R2 = FP;
	CC = R1 == R2;
	IF !CC JUMP 1f;

	/* make sure FP/RETS on the stack have our known values */
	R1 = [SP];
	CC = R1 == R6;
	IF !CC JUMP 1f;

	R1 = [SP + 4];
	CC = R1 == R7;
	IF !CC JUMP 1f;

	/* UNLINK should:
	 *	assign SP to current FP
	 *	adjust SP by -8 bytes
	 *	restore RETS/FP from the stack
	 */
	R4 = 0;
	RETS = R4;
	R0 = SP;
	UNLINK;

	/* Check new SP */
	R1 = SP;
	R1 += -0x8;
	CC = R1 == R0;
	IF !CC JUMP 1f;

	/* Check restored RETS */
	R1 = RETS;
	CC = R1 == R7;
	IF !CC JUMP 1f;

	/* Check restored FP */
	R1 = FP;
	CC = R1 == R6;
	IF !CC JUMP 1f;

	pass
1:
	fail
