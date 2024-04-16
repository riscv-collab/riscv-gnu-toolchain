//Original:/proj/frio/dv/testcases/core/c_ccmv_ncc_dr_pr/c_ccmv_ncc_dr_pr.dsp
// Spec Reference: ccmv !cc dpreg = dpreg
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	imm32 r0, 0x138d2301;
	imm32 r1, 0x20421053;
	imm32 r2, 0x3f051405;
	imm32 r3, 0x40b66507;
	imm32 r4, 0x50487709;
	imm32 r5, 0x6005908b;
	imm32 r6, 0x7a0c6609;
	imm32 r7, 0x890e108f;
	imm32 p1, 0x9d021053;
	imm32 p2, 0xafb41405;
	imm32 p3, 0xb0bf1507;
	imm32 p4, 0xd0483609;
	imm32 p5, 0xe005d00b;
	imm32 sp, 0xfa0c667d;
	imm32 fp, 0xc90e108f;
	IF !CC R0 = P0;
	CC = ! CC;
	IF !CC P1 = R3;
	IF !CC R2 = P5;
	IF !CC P2 = R2;
	IF !CC P3 = R6;
	IF !CC R5 = P1;
	CC = ! CC;
	IF !CC P4 = R7;
	IF !CC R7 = P4;
	IF !CC P5 = R3;
	IF !CC R6 = SP;
	CC = ! CC;
	IF !CC R3 = P2;
	IF !CC SP = R6;
	IF !CC R1 = P5;
	CC = ! CC;
	IF !CC FP = R4;
	IF !CC R3 = P3;
	CHECKREG r1, 0x20421053;
	CHECKREG r2, 0x3F051405;
	CHECKREG r3, 0xB0BF1507;
	CHECKREG r4, 0x50487709;
	CHECKREG r5, 0x6005908B;
	CHECKREG r6, 0xFA0C667D;
	CHECKREG r7, 0x890E108F;
	CHECKREG p1, 0x9D021053;
	CHECKREG p2, 0xAFB41405;
	CHECKREG p3, 0xB0BF1507;
	CHECKREG p4, 0x890E108F;
	CHECKREG p5, 0x40B66507;
	CHECKREG sp, 0xFA0C667D;
	CHECKREG fp, 0x50487709;

	pass
