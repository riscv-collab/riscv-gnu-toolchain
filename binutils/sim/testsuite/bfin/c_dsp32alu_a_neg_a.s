//Original:/testcases/core/c_dsp32alu_a_neg_a/c_dsp32alu_a_neg_a.dsp
// Spec Reference: dsp32alu a = neg a
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
A1 = A0 = 0;
A0 = R0;

A0 = - A0;
A1 = - A0;
A1 = - A1;
A0 = - A1;
R1 = A0.w;
R2 = A1.w;
CHECKREG r0, 0xA5678911;
CHECKREG r1, 0xA5678911;
CHECKREG r2, 0x5A9876EF;


pass
