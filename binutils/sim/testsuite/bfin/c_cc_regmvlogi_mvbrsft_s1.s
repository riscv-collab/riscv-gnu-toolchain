//Original:/testcases/core/c_cc_regmvlogi_mvbrsft_s1/c_cc_regmvlogi_mvbrsft_s1.dsp
// Spec Reference: cc: set (regmv & logi2op) used (ccmv & brcc & dsp32sft)
# mach: bfin

.include "testutils.inc"
	start


A0 = 0;
A1 = 0;

imm32 r0, 0x00000020;	// cc=1
imm32 r1, 0x00000000;	// cc=0
imm32 r2, 0x62b61557;
imm32 r3, 0x07300007;
imm32 r4, 0x00740088;
imm32 r5, 0x609950aa;
imm32 r6, 0x20bb06cc;
imm32 r7, 0x00000002;


ASTAT = R0;	// cc=1 REGMV
R5 = R0 + R2;	// comp3op dr plus dr
IF CC R1 = R3;	// ccmov
ASTAT = R1;	// cc=0 REGMV
R4 >>= R7;	// alu2op sft
IF CC R3 = R2;	// ccmv
CC = R0 < R1;	// ccflag
R3.H = R1.L + R3.H (S);	// dsp32alu
IF CC R4 = R5;	// ccmv
CC = ! BITTST( R0 , 4 );	// cc = 0
R1 = R0 +|- R1 , R6 = R0 -|+ R1 (ASR);	// dsp32alu sft
IF CC R4 = R5;	// ccmv
CC = BITTST ( R1 , 4 );	// cc = 0
R3.L = R5.L << 1;	// dsp32shiftim
IF !CC JUMP LABEL1;	// branch
CC = ! CC;
R1 = ( A1 = R7.L * R4.L ), R0 = ( A0 = R7.H * R4.H ) (S2RND);	// dsp32mac pair
IF !CC JUMP LABEL2 (BP);	// branch
LABEL1:
 R2 = R0 + R2;
JUMP.S END;
LABEL2:
 R7 = R5 - R3;
CC = R0 < R1;	// ccflag
R5 = R0 + R2;	// comp3op dr plus dr
IF CC JUMP END (BP);	// branch on
R4 = R5 + R7;

END:

CHECKREG r0, 0x00000020;
CHECKREG r1, 0x0398000C;
CHECKREG r2, 0x62B61577;
CHECKREG r3, 0x07372AEE;
CHECKREG r4, 0x62B61577;
CHECKREG r5, 0x62B61577;
CHECKREG r6, 0xFC680013;
CHECKREG r7, 0x00000002;

imm32 r0, 0x00000020;
imm32 r1, 0x00000000;
imm32 r2, 0x62661557;
imm32 r3, 0x073b0007;
imm32 r4, 0x01f49088;
imm32 r5, 0x6e2959aa;
imm32 r6, 0xa0b506cc;
imm32 r7, 0x00000002;


	ASTAT = R0;				// cc=1 REGMV
	R4.H = R1.L + R0.L (S);			// dsp32alu
	R2 = ROT R2 BY 1;			// dsp32shiftim_rot
	ASTAT = R1;				// cc=0 REGMV
	A1 = R2.L * R3.L, A0 += R2.L * R3.H;	// dsp32mac
	R3 = ROT R3 BY 1;			// dsp32shiftim_rot
	CC = ! BITTST( R0 , 4 );		// cc = 0
	R4.L = R5.L << 1;			// dsp32shiftimm
	R6 = ROT R4 BY 5;			// dsp32shiftim_rot
	CC = BITTST ( R1 , 4 );			// cc = 0
	R7 = R0 + R2;				// comp3op dr plus dr
	IF CC R4 = R5;				// ccmov
	A0 += A1 (W32);				// dsp32alu a0 + a1
	CC = BITTST ( R0 , 4 );			// cc = 1
	R5 = ROT R6 BY R7.L;
	R0 = A0.w;
	R1 = A1.w;

CHECKREG r0, 0x026B943C;
CHECKREG r1, 0x00025592;
CHECKREG r2, 0xC4CC2AAF;
CHECKREG r3, 0x0E76000E;
CHECKREG r4, 0x0020B354;
CHECKREG r5, 0x35480105;
CHECKREG r6, 0x04166A90;
CHECKREG r7, 0xC4CC2ACF;

pass
