//Original:/testcases/core/c_brcc_brf_fbkwd/c_brcc_brf_fbkwd.dsp
// Spec Reference: brcc brf forward/backward
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

ASTAT = R0;

IF !CC JUMP SUBR;
 R1.L = 0xeeee;
 R2.L = 0x2222;
 R3.L = 0x3333;
JBACK:
 R4.L = 0x4444;




CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00001111;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00004444;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

pass

//.code 0x448
SUBR:
 R1.L = 0x1111;
IF !CC JUMP JBACK;
