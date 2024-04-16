//Original:/proj/frio/dv/testcases/core/c_dsp32shiftim_rot/c_dsp32shiftim_rot.dsp
// Spec Reference: dsp32shiftimm rot:
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;


	imm32 r0, 0xa1230001;
	imm32 r1, 0x1b345678;
	imm32 r2, 0x23c56789;
	imm32 r3, 0x34d6789a;
	imm32 r4, 0x85a789ab;
	imm32 r5, 0x967c9abc;
	imm32 r6, 0xa789abcd;
	imm32 r7, 0xb8912cde;
	R0 = ROT R0 BY 1;
	R1 = ROT R1 BY 5;
	R2 = ROT R2 BY 9;
	R3 = ROT R3 BY 8;
	R4 = ROT R4 BY 24;
	R5 = ROT R5 BY 31;
	R6 = ROT R6 BY 14;
	R7 = ROT R7 BY 25;
	CHECKREG r0, 0x42460002;
	CHECKREG r1, 0x668ACF11;
	CHECKREG r2, 0x8ACF1323;
	CHECKREG r3, 0xD6789A9A;
	CHECKREG r4, 0xAB42D3C4;
	CHECKREG r5, 0x659F26AF;
	CHECKREG r6, 0x6AF354F1;
	CHECKREG r7, 0xBCB8912C;

	imm32 r0, 0xa1230001;
	imm32 r1, 0x1b345678;
	imm32 r2, 0x23c56789;
	imm32 r3, 0x34d6789a;
	imm32 r4, 0x85a789ab;
	imm32 r5, 0x967c9abc;
	imm32 r6, 0xa789abcd;
	imm32 r7, 0xb8912cde;
	R6 = ROT R0 BY -3;
	R7 = ROT R1 BY -9;
	R0 = ROT R2 BY -8;
	R1 = ROT R3 BY -7;
	R2 = ROT R4 BY -15;
	R3 = ROT R5 BY -24;
	R4 = ROT R6 BY -31;
	R5 = ROT R7 BY -22;
	CHECKREG r0, 0x1223C567;
	CHECKREG r1, 0x6A69ACF1;
	CHECKREG r2, 0x26AD0B4F;
	CHECKREG r3, 0xF9357896;
	CHECKREG r4, 0xD0918000;
	CHECKREG r5, 0x6CD15DE0;
	CHECKREG r6, 0x74246000;
	CHECKREG r7, 0x780D9A2B;

	pass
