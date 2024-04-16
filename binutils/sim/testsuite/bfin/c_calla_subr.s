//Original:/testcases/core/c_calla_subr/c_calla_subr.dsp
// Spec Reference: progctrl calla subr
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

CALL SUBR;

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
	R2.L = 0x2222;		// should not go here
	RTS;
