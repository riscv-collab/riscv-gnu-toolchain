//Original:/testcases/core/c_brcc_brf_brt_bp/c_brcc_brf_brt_bp.dsp
// Spec Reference: brcc brfbrt
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x00000000;
imm32 r1, 0x00000000;
imm32 r2, 0x00000000;
imm32 r3, 0x00000000;
imm32 r4, 0x00000444;
imm32 r5, 0x00000555;
imm32 r6, 0x00000000;
imm32 r7, 0x00000000;

begin:
	ASTAT = R0;		// clear cc
	CC = R4 < R5;
	IF CC JUMP good1 (BP);	// branch on true (should branch)
	R1 = 1;			// if go here, error
good1:	IF !CC JUMP bad1 (BP);	// branch on false (should not branch)
	CC = ! CC;
	IF !CC JUMP good2;	// should branch here
bad1:	R2 = 2;			// if go here, error
good2:	CC = ! CC;		// clear cc=0
	IF CC JUMP good3 (BP);	// branch on false (should branch)
	R3 = 3;			// if go here, error
good3:	IF !CC JUMP bad2 (BP);	// branch on true (should not branch)
	IF CC JUMP end;		// we're done
bad2:	R0 = 8;			// if go here error

end:

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000444;
CHECKREG r5, 0x00000555;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

pass
