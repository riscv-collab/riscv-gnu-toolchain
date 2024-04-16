//Original:/testcases/core/c_dsp32alu_rrpm_aa/c_dsp32alu_rrpm_aa.dsp
// Spec Reference: dsp32alu (dregs, dregs) = +/- (a, a) amod1
# mach: bfin

.include "testutils.inc"
	start



A1 = A0 = 0;

imm32 r0, 0x75678911;
imm32 r1, 0xa789ab2d;
imm32 r2, 0x34745515;
imm32 r3, 0x46677757;
imm32 r4, 0xb567a96b;
imm32 r5, 0x6789aa1d;
imm32 r6, 0x744455a5;
imm32 r7, 0x8666777a;
A0 = R0;
A1 = R1;

R0 = A1 + A0, R7 = A1 - A0 (NS);
R1 = A0 + A1, R6 = A0 - A1 (NS);
R2 = A1 + A0, R5 = A1 - A0 (NS);
R3 = A0 + A1, R4 = A0 - A1 (NS);
R4 = A1 + A0, R0 = A1 - A0 (NS);
R5 = A0 + A1, R1 = A0 - A1 (NS);
R6 = A0 + A1, R2 = A0 - A1 (NS);
R7 = A1 + A0, R3 = A1 - A0 (NS);
CHECKREG r0, 0x3222221C;
CHECKREG r1, 0xCDDDDDE4;
CHECKREG r2, 0xCDDDDDE4;
CHECKREG r3, 0x3222221C;
CHECKREG r4, 0x1CF1343E;
CHECKREG r5, 0x1CF1343E;
CHECKREG r6, 0x1CF1343E;
CHECKREG r7, 0x1CF1343E;

imm32 r0, 0x8537891b;
imm32 r1, 0x3759ab2d;
imm32 r2, 0x4e555535;
imm32 r3, 0x16e65747;
imm32 r4, 0x687e9565;
imm32 r5, 0x7a8aeb5b;
imm32 r6, 0x8c9cdd85;
imm32 r7, 0x9eaefe9f;
A0 = R0;
A1 = R1;
R3 = A1 + A0, R7 = A1 - A0 (S);
R4 = A0 + A1, R6 = A0 - A1 (S);
R5 = A1 + A0, R4 = A1 - A0 (S);
R6 = A0 + A1, R5 = A0 - A1 (S);
R7 = A1 + A0, R3 = A1 - A0 (S);
R0 = A0 + A1, R2 = A0 - A1 (S);
R1 = A0 + A1, R0 = A0 - A1 (S);
R2 = A1 + A0, R1 = A1 - A0 (S);
CHECKREG r0, 0x80000000;
CHECKREG r1, 0x7FFFFFFF;
CHECKREG r2, 0xBC913448;
CHECKREG r3, 0x7FFFFFFF;
CHECKREG r4, 0x7FFFFFFF;
CHECKREG r5, 0x80000000;
CHECKREG r6, 0xBC913448;
CHECKREG r7, 0xBC913448;




pass
