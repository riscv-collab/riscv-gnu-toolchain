//Original:/testcases/core/c_brcc_bp1/c_brcc_bp1.dsp
// Spec Reference: brcc bp
# mach: bfin

.include "testutils.inc"
	start



imm32 r0, 0x00000000;
imm32 r1, 0x00000000;
imm32 r2, 0x00000000;
imm32 r3, 0x00000000;
imm32 r4, 0x00000000;
imm32 r5, 0x00000000;
imm32 r6, 0x00000000;
imm32 r7, 0x00000000;

begin:
ASTAT = R0;			// clear cc
	CC = ! CC;		// set cc=1
	IF CC JUMP good1 (BP);	// branch on true (should branch)
	R1 = 1;			// if go here, error
good1:	IF !CC JUMP bad1;	// branch on false (should not branch)
	JUMP.S good2;		// should branch here
bad1:	R2 = 2;			// if go here, error
good2:	CC = ! CC;		// clear cc=0
	IF !CC JUMP good3;	// branch on false (should branch)
	R3 = 3;			// if go here, error
good3:	IF CC JUMP bad2;	// branch on true (should not branch)
	JUMP.S end;		// we're done
bad2:	R4 = 4;			// if go here error

end:

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

pass
