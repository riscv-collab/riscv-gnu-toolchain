//Original:/testcases/core/c_dsp32alu_sgn/c_dsp32alu_sgn.dsp
// Spec Reference: dsp32alu dreg_lo(hi) = rnd dregs
# mach: bfin

.include "testutils.inc"
	start

imm32 r0, 0x456789ab;
imm32 r1, 0x6689abcd;
imm32 r2, 0x47445555;
imm32 r3, 0x68667777;
R4.H = R4.L = SIGN(R2.H) * R0.H + SIGN(R2.L) * R0.L;
R5.H = R5.L = SIGN(R2.H) * R1.H + SIGN(R2.L) * R1.L;
R6.H = R6.L = SIGN(R2.H) * R2.H + SIGN(R2.L) * R2.L;
R7.H = R7.L = SIGN(R2.H) * R3.H + SIGN(R2.L) * R3.L;
CHECKREG r4, 0xCF12CF12;
CHECKREG r5, 0x12561256;
CHECKREG r6, 0x9C999C99;
CHECKREG r7, 0xDFDDDFDD;

imm32 r0, 0x496789ab;
imm32 r1, 0x6489abcd;
imm32 r2, 0x4b445555;
imm32 r3, 0x6c647777;
imm32 r4, 0x8d889999;
imm32 r5, 0xaeaa4bbb;
imm32 r6, 0xcfccd44d;
imm32 r7, 0xe1eefff4;
R0.H = R0.L = SIGN(R3.H) * R4.H + SIGN(R3.L) * R4.L;
R1.H = R1.L = SIGN(R3.H) * R5.H + SIGN(R3.L) * R5.L;
R2.H = R2.L = SIGN(R3.H) * R6.H + SIGN(R3.L) * R6.L;
R3.H = R3.L = SIGN(R3.H) * R7.H + SIGN(R3.L) * R7.L;
CHECKREG r0, 0x27212721;
CHECKREG r1, 0xFA65FA65;
CHECKREG r2, 0xA419A419;
CHECKREG r3, 0xE1E2E1E2;


pass
