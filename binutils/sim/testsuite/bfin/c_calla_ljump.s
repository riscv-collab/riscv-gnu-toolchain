//Original:/testcases/core/c_calla_ljump/c_calla_ljump.dsp
// Spec Reference: progctrl calla ljump
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

JUMP.L SUBR;

JBACK:

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
	JUMP.L JBACK;
	R2.L = 0x2222;		// should not go here
	JUMP.L JBACK;
RTS;
