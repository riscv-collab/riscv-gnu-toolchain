# Blackfin testcase for negate instruction
# mach: bfin

#include "test.h"

	.include "testutils.inc"

	start

	.global _test
_test:
	R6 = ASTAT;
	R0.H = 0x8000;
	R0.L = 0x0000;
	R1 = -R0;
	R7 = ASTAT;
	R2.H = 0x8000;
	R2.L = 0x0000;
	CC = R1 == R2;
	IF !CC JUMP 1f;
	/* CLEARED: AZ AC0 AC0_COPY */
	R3.H = HI(_AZ|_AC0|_AC0_COPY);
	R3.L = LO(_AZ|_AC0|_AC0_COPY);
	R4 = R7 & R3;
	CC = R4 == 0;
	IF !CC JUMP 1f;
	/* SET: AN V V_COPY VS */
	R3.H = HI(_AN|_V|_V_COPY|_VS);
	R3.L = LO(_AN|_V|_V_COPY|_VS);
	R4 = R7 & R3;
	CC = R3 == R4;
	IF !CC JUMP 1f;
	/* UNAFFECTED: CC AQ RND_MOD AV0 AV0S AV1 AV1S AC1 */
	R3.H = HI(_CC|_AQ|_RND_MOD|_AV0|_AV0S|_AV1|_AV1S|_AC1);
	R3.L = LO(_CC|_AQ|_RND_MOD|_AV0|_AV0S|_AV1|_AV1S|_AC1);
	R4 = R6 & R3;
	R5 = R7 & R3;
	CC = R4 == R5;
	IF !CC JUMP 1f;
	pass
1:
	fail
