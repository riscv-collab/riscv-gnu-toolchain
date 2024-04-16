//Original:/proj/frio/dv/testcases/core/c_cc_flag_ccmv_depend/c_cc_flag_ccmv_depend.dsp
// Spec Reference: ccflag followed by ccmv (# stalls)
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;
	imm32 r0, 0xa08d2311;
	imm32 r1, 0x10120040;
	imm32 r2, 0x62b61557;
	imm32 r3, 0x07300007;
	imm32 r4, 0x00740088;
	imm32 r5, 0x609950aa;
	imm32 r6, 0x20bb06cc;
	imm32 r7, 0xd90e108f;

	imm32 p1, 0x1401101f;
	imm32 p2, 0x3204108e;
	imm32 fp, 0xd93f1084;
	imm32 p4, 0xeb04106f;
	imm32 p5, 0xa90e5089;

	CC = R7;	// cc2dreg
	IF CC R0 = R3;	// ccmov
	R6 = R0 + R4;

	CC = ! CC;	// cc2dreg
	IF CC R1 = P1;	// ccmov

	CC = R5 < R1;	// ccflag
	R1 = ASTAT;
	IF !CC R2 = R5;	// ccmov

	CC = R2 == R3;	// ccflag
	IF CC P1 = R4;	// ccmov

	CC = ! CC;
	CC = R7 < R5;
	IF CC P2 = P5;	// ccmov

	CC = P5 == 3;
	IF CC FP = R2;	// ccmov

	R3 = ( A1 = R7.L * R4.L ),  R2 = ( A0 = R7.H * R4.H )  (S2RND);	// dsp32mac pair

	CC = A0 == A1;
	IF !CC R3 = R6;	// ccmov
	R7 = R3 + R2;

	A0 += A1 (W32);	// dsp32alu a0 + a1
	CC = A0 < A1;
	IF CC R4 = P4;	// ccmov
	R6 = R4;

	R2.H = ( A1 = R2.L * R3.L ), A0 = R2.H * R3.L;	// dsp32mac
	CC = A0 <= A1;
	IF CC R5 = P5;	// ccmov

	A1 = R2.L * R3.L, A0 += R2.L * R3.H;	// dsp32mac
	CC = A0 <= A1;
	IF CC P5 = R6;	// ccmov

	CHECKREG r0, 0x07300007;
	CHECKREG r1, (_AC0|_AC0_COPY);
	CHECKREG r2, 0x00766960;
	CHECKREG r3, 0x07A4008F;
	CHECKREG r4, 0xEB04106F;
	CHECKREG r5, 0xA90E5089;
	CHECKREG r6, 0xEB04106F;
	CHECKREG r7, 0x075D69EF;
	CHECKREG p1, 0x1401101F;
	CHECKREG p2, 0xA90E5089;
	CHECKREG fp, 0xD93F1084;
	CHECKREG p4, 0xEB04106F;
	CHECKREG p5, 0xA90E5089;

	pass
