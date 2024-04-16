//Original:/testcases/core/c_dsp32alu_aa_absabs/c_dsp32alu_aa_absabs.dsp
// Spec Reference: dsp32alu a1, a0 = abs / abs a1, a0
# mach: bfin

.include "testutils.inc"
	start





imm32 r0, 0xa5678911;
imm32 r1, 0x2789ab1d;
imm32 r2, 0x3b44b515;
imm32 r3, 0x46667717;
imm32 r4, 0x5567891b;
imm32 r5, 0x6789ab1d;
imm32 r6, 0x74445515;
imm32 r7, 0x86667777;
A0 = R0;
A1 = R1;

A1 = ABS A1, A0 = ABS A0;
R2 = A0.w;
R3 = A1.w;
A1 = ABS A1, A0 = ABS A0;
R4 = A0.w;
R5 = A1.w;
CHECKREG r2, 0x5A9876EF;
CHECKREG r3, 0x2789AB1D;
CHECKREG r4, 0x5A9876EF;
CHECKREG r5, 0x2789AB1D;


pass
