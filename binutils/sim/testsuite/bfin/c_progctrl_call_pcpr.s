//Original:/testcases/core/c_progctrl_call_pcpr/c_progctrl_call_pcpr.dsp
// Spec Reference: progctrl call (pc+pr)
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;

	ASTAT = r0;

	FP = SP;

	P2 = 0x0006;

JMP:
	CALL ( PC + P2 );
	JUMP.S JMP;

STOP:
	JUMP.S END;

LAB1:
	P2 = 0x000e;
	R1 = 0x1111 (X);
	RTS;

LAB2:
	P2 = 0x0016;
	R2 = 0x2222 (X);
	RTS;

LAB3:
	P2 = 0x001e;
	R3 = 0x3333 (X);
	RTS;

LAB4:
	P2 = 0x0026;
	R4 = 0x4444 (X);
	RTS;

LAB5:
	P2 = 0x0004;
	R5 = 0x5555 (X);
	RTS;

END:

	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x00001111;
	CHECKREG r2, 0x00002222;
	CHECKREG r3, 0x00003333;
	CHECKREG r4, 0x00004444;
	CHECKREG r5, 0x00005555;
	CHECKREG r6, 0x00000000;
	CHECKREG r7, 0x00000000;

	pass

	.data
DATA:
	.space (0x0100);
