//Original:/testcases/core/c_progctrl_call_pr/c_progctrl_call_pr.dsp
// Spec Reference: progctrl call (pr)
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;

	ASTAT = r0;

	FP = SP;

	loadsym P1, SUBR;
	CALL ( P1 );

	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x00001111;
	CHECKREG r2, 0x00000000;
	CHECKREG r3, 0x00000000;
	CHECKREG r4, 0x00000000;
	CHECKREG r5, 0x00000000;
	CHECKREG r6, 0x00000000;
	CHECKREG r7, 0x00000000;

	pass

SUBR:				// should jump here
	R1.L = 0x1111;
	RTS;
	R2.L = 0x2222;	// should not go here
	RTS;
