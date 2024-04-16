//Original:/proj/frio/dv/testcases/core/c_progctrl_rts/c_progctrl_rts.dsp
// Spec Reference: progctrl rts
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;

	ASTAT = r0;

	loadsym R2, SUBR;
	RETS = R2;
	RTS;

STOP:

	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x00000000;
	CHECKREG r4, 0x00004444;
	CHECKREG r5, 0x00000000;
	CHECKREG r6, 0x00000000;
	CHECKREG r7, 0x00000000;

	pass

SUBR:				// should jump here
	loadsym R3, STOP;
	RETS = R3;
	R4.L = 0x4444;
	RTS;
	RETS = R3;
	R5.L = 0x5555;	// should not go here
	RTS;

	fail
