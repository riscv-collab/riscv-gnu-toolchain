//Original:/testcases/core/c_dsp32alu_rr_lph_a1a0/c_dsp32alu_rr_lph_a1a0.dsp
// Spec Reference: dsp32alu (dregs, dregs) = L + H, L + H (a1, a0)
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x25678911;
imm32 r1, 0x0029ab2d;
imm32 r2, 0x00145535;
imm32 r3, 0xf6567747;
imm32 r4, 0xe566895b;
imm32 r5, 0x67897b6d;
imm32 r6, 0xb4445875;
imm32 r7, 0x86667797;
A1 = R1;
A0 = R0;

R2 = A1.L + A1.H, R3 = A0.L + A0.H;
R4 = A1.L + A1.H, R5 = A0.L + A0.H;
R6 = A1.L + A1.H, R7 = A0.L + A0.H;
CHECKREG r2, 0xFFFFAB56;
CHECKREG r3, 0xFFFFAE78;
CHECKREG r4, 0xFFFFAB56;
CHECKREG r5, 0xFFFFAE78;
CHECKREG r6, 0xFFFFAB56;
CHECKREG r7, 0xFFFFAE78;


pass
