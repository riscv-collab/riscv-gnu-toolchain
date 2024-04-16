//Original:/testcases/core/c_dsp32alu_a0_pm_a1/c_dsp32alu_a0_pm_a1.dsp
// Spec Reference: dsp32alu a0 += a1
# mach: bfin

.include "testutils.inc"
	start



A1 = A0 = 0;

imm32 r0, 0x25678911;
imm32 r1, 0x0029ab2d;
imm32 r2, 0x00145535;
imm32 r3, 0xf6567747;
imm32 r4, 0xe566895b;
imm32 r5, 0x67897b6d;
imm32 r6, 0xb4445875;
imm32 r7, 0x86667797;
A0 = R0;
A1 = R1;

A0 += A1;
A0 += A1 (W32);
A0 += A1;
A0 += A1 (W32);
R5 = A0.w;

A1 = R2;
A0 -= A1;
A0 -= A1 (W32);
A0 -= A1;
A0 -= A1 (W32);
R6 = A0.w;
CHECKREG r5, 0x260E35C5;
CHECKREG r6, 0x25BCE0F1;


pass
