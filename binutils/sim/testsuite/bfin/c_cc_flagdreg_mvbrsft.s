//Original:/testcases/core/c_cc_flagdreg_mvbrsft/c_cc_flagdreg_mvbrsft.dsp
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

	ASTAT = R0;

	CC = R1;			// cc2dreg
	IF CC R1 = R3;			// ccmov
	CC = ! CC;			// cc2dreg
	IF CC R3 = R2;			// ccmov
	CC = R0 < R1;			// ccflag
	IF CC R4 = R5;			// ccmov
	CC = R2 == R3;
	IF CC R4 = R5;			// ccmov
	CC = R0;			// cc2dreg
	IF !CC JUMP LABEL1;		// branch on
	CC = ! CC;
	IF !CC JUMP LABEL2 (BP);	// branch on
LABEL1:
	R6 = R0 + R2;
	JUMP.S END;
LABEL2:
	R7 = R5 - R3;
	CC = R0 < R1;			// ccflag
	IF CC JUMP END (BP);		// branch on
	R4 = R5 + R7;

END:

CHECKREG r0, 0xA08D2311;
CHECKREG r1, 0x07300007;
CHECKREG r2, 0x62B61557;
CHECKREG r3, 0x07300007;
CHECKREG r4, 0x609950AA;
CHECKREG r5, 0x609950AA;
CHECKREG r6, 0x20BB06CC;
CHECKREG r7, 0x596950A3;

imm32 r0, 0x408d2711;
imm32 r1, 0x15124040;
imm32 r2, 0x62661557;
imm32 r3, 0x073b0007;
imm32 r4, 0x01f49088;
imm32 r5, 0x6e2959aa;
imm32 r6, 0xa0b506cc;
imm32 r7, 0x00000002;


	CC = R1;		// cc2dreg
	R2 = ROT R2 BY 1;	// dsp32shiftim_rot
	CC = ! CC;		// cc2dreg
	R3 = ROT R0 BY -3;	// dsp32shiftim_rot
	CC = R0 < R1;		// ccflag
	R6 = ROT R4 BY 5;	// dsp32shiftim_rot
	CC = R2 == R3;
	IF CC R4 = R5;		// ccmov
	CC = R0;		// cc2dreg
	R7 = ROT R6 BY R7.L;

CHECKREG r0, 0x408D2711;
CHECKREG r1, 0x15124040;
CHECKREG r2, 0xC4CC2AAF;
CHECKREG r3, 0x6811A4E2;
CHECKREG r4, 0x01F49088;
CHECKREG r5, 0x6E2959AA;
CHECKREG r6, 0x3E921100;
CHECKREG r7, 0xFA484402;




pass
