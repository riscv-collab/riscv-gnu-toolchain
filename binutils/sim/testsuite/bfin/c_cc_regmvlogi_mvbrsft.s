//Original:/testcases/core/c_cc_regmvlogi_mvbrsft/c_cc_regmvlogi_mvbrsft.dsp
// Spec Reference: cc: set (regmv & logi2op) used (ccmv & brcc & dsp32sft)
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x00000020;	// cc=1
imm32 r1, 0x00000000;	// cc=0
imm32 r2, 0x62b61557;
imm32 r3, 0x07300007;
imm32 r4, 0x00740088;
imm32 r5, 0x609950aa;
imm32 r6, 0x20bb06cc;
imm32 r7, 0xd90e108f;


	ASTAT = R0;			// cc=1 REGMV
	IF CC R1 = R3;			// ccmov
	ASTAT = R1;			// cc=0 REGMV
	IF CC R3 = R2;			// ccmv
	CC = R0 < R1;			// ccflag
	IF CC R4 = R5;			// ccmv
	CC = ! BITTST( R0 , 4 );	// cc = 0
	IF CC R4 = R5;			// ccmv
	CC = BITTST ( R1 , 4 );		// cc = 0
	IF !CC JUMP LABEL1;		// branch
	CC = ! CC;
	IF !CC JUMP LABEL2 (BP);	// branch
LABEL1:
	R6 = R0 + R2;
	JUMP.S END;
LABEL2:
	R7 = R5 - R3;
	CC = R0 < R1;			// ccflag
	IF CC JUMP END (BP);		// branch on
	R4 = R5 + R7;

END:

CHECKREG r0, 0x00000020;
CHECKREG r1, 0x07300007;
CHECKREG r2, 0x62B61557;
CHECKREG r3, 0x07300007;
CHECKREG r4, 0x609950AA;
CHECKREG r5, 0x609950AA;
CHECKREG r6, 0x62B61577;
CHECKREG r7, 0xD90E108F;

imm32 r0, 0x00000020;
imm32 r1, 0x00000000;
imm32 r2, 0x62661557;
imm32 r3, 0x073b0007;
imm32 r4, 0x01f49088;
imm32 r5, 0x6e2959aa;
imm32 r6, 0xa0b506cc;
imm32 r7, 0x00000002;


	ASTAT = R0;			// cc=1 REGMV
	R2 = ROT R2 BY 1;		// dsp32shiftim_rot
	ASTAT = R1;			// cc=0 REGMV
	R3 = ROT R3 BY 1;		// dsp32shiftim_rot
	CC = ! BITTST( R0 , 4 );	// cc = 0
	R6 = ROT R4 BY 5;		// dsp32shiftim_rot
	CC = BITTST ( R1 , 4 );		// cc = 0
	IF CC R4 = R5;			// ccmov
	CC = BITTST ( R0 , 4 );		// cc = 1
	R7 = ROT R6 BY R7.L;

CHECKREG r0, 0x00000020;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0xC4CC2AAF;
CHECKREG r3, 0x0E76000E;
CHECKREG r4, 0x01F49088;
CHECKREG r5, 0x6E2959AA;
CHECKREG r6, 0x3E921110;
CHECKREG r7, 0xFA484440;

pass
