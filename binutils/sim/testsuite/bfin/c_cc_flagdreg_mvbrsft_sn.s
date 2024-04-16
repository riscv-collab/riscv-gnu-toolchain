//Original:/proj/frio/dv/testcases/core/c_cc_flagdreg_mvbrsft_sn/c_cc_flagdreg_mvbrsft_sn.dsp
// Spec Reference: cc: set (ccflag & cc2dreg) used (ccmv & brcc & dsp32sft)
# mach: bfin

.include "testutils.inc"
	start

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
	imm32 p3, 0xd93f1084;
	imm32 p4, 0xeb04106f;
	imm32 p5, 0xa90e5089;

	ASTAT = R0;

	CC = R1;	// cc2dreg
	R2.H = ( A1 = R2.L * R3.L ), A0 = R2.H * R3.L;	// dsp32mac
	I0 = P1;	// regmv
	IF CC R1 = R3;	// ccmov
	CC = ! CC;	// cc2dreg
	R4.H = R1.L + R0.L (S);	// dsp32alu
	M0 = P2;	// regmv
	IF CC R3 = R2;	// ccmov
	CC = R0 < R1;	// ccflag
	R4.L = R5.L << 1;	// dsp32shiftimm
	I0 += M0;	// dagmodim
	R2 = R0 + R2;	// comp3op dr plus dr
	IF CC R4 = R5;	// ccmov
	CC = R2 == R3;	// ccflag
	R7 = R1.L * R4.L, R6 = R1.H * R4.H;	// dsp32mult
	R5 = R0 + R2;	// comp3op dr plus dr
	BITCLR( R6 , 1 );
	IF CC R4 = R5;	// ccmov
	CC = R0;	// cc2dreg
	A1 = R2.L * R3.L, A0 += R2.L * R3.H;	// dsp32mac
	IF !CC JUMP LABEL1;	// branch on
	CC = ! CC;	// cc2dreg
	P1.L = 0x3000;	// ldimmhalf
	A0 += A1 (W32);	// dsp32alu a0 + a1
	IF !CC JUMP LABEL2 (BP);	// branch
LABEL1:
	R6 = R6 + R2;
	JUMP.S END;
LABEL2:
	R7 = R5 - R7;
	CC = R0 < R1;	// ccflag
	P2 = A0.w;
	IF CC JUMP END (BP);	// branch
	P3 = A1.w;
	R5 = R5 + R7;

END:

	CHECKREG r0, 0xA08D2311;
	CHECKREG r1, 0x07300007;
	CHECKREG r2, 0xA08E3868;
	CHECKREG r3, 0x07300007;
	CHECKREG r4, 0x609950AA;
	CHECKREG r5, 0x411B5B79;
	CHECKREG r6, 0x056C9760;
	CHECKREG r7, 0x4116F22D;
	CHECKREG p1, 0x14013000;
	CHECKREG p2, 0x033352A4;
	CHECKREG p3, 0xD93F1084;

	imm32 r0, 0x408d2711;
	imm32 r1, 0x15124040;
	imm32 r2, 0x62661557;
	imm32 r3, 0x073b0007;
	imm32 r4, 0x01f49088;
	imm32 r5, 0x6e2959aa;
	imm32 r6, 0xa0b506cc;
	imm32 r7, 0x00000002;

	CC = R1;	// cc2dreg
	P1 = -15;	// compi2opp_pr_eq_i7
	R2 = ROT R2 BY 1;	// dsp32shiftim_rot
	CC = ! CC;	// cc2dreg
	R3 >>= R7;	// alu2op sft
	R4 = ROT R0 BY -3;	// dsp32shiftim_rot
	CC = R0 < R1;	// ccflag
	R3 = ( A1 = R7.L * R4.L ),  R2 = ( A0 = R7.H * R4.H )  (S2RND);	// dsp32mac pair
	R5 = R0 + R2;	// comp3op dr plus dr
	R6 = ROT R4 BY 5;	// dsp32shiftim_rot
	CC = R2 == R3;	// ccflag
	P2 = R1;	// regmv
	R4.H = R1.L + R3.H (S);	// dsp32alu
	I0 = P1;	// regmv
	IF CC R4 = R5;	// ccmov
	CC = R0;	// cc2dreg
	R1 = R0 +|- R1 , R6 = R0 -|+ R1 (ASR);	// dsp32alu sft
	I0 += 2;
	P3 = I0;
	R3.L = R5.L << 1;	// dsp32shiftimm
	R7 = ROT R6 BY R7.L;	// dsp32shiftim_rot

	CHECKREG r0, 0x408D2711;
	CHECKREG r1, 0x2ACFF368;
	CHECKREG r2, 0x00000000;
	CHECKREG r3, 0xFFFD4E22;
	CHECKREG r4, 0x403DA4E2;
	CHECKREG r5, 0x408D2711;
	CHECKREG r6, 0x15BD33A8;
	CHECKREG r7, 0x56F4CEA2;
	CHECKREG p1, 0xFFFFFFF1;
	CHECKREG p2, 0x15124040;
	CHECKREG p3, 0xFFFFFFF3;

	pass
