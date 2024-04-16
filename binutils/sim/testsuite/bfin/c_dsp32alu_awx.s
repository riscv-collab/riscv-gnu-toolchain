//Original:/testcases/core/c_dsp32alu_awx/c_dsp32alu_awx.dsp
// Spec Reference: dsp32alu awx
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x15678911;
imm32 r1, 0x2789ab1d;
imm32 r2, 0x34445515;
imm32 r3, 0x46667717;
imm32 r4, 0x5567891b;
imm32 r5, 0x6789ab1d;
imm32 r6, 0x74445515;
imm32 r7, 0x86667777;
// A0 & A1 types
A0 = 0;
A1 = 0;

A0.L = R0.L;
A0.H = R0.H;
A0.x = R2.L;
R3 = A0.w;
R4 = A1.w;
R5.L = A0.x;
//rl6 = a1x;
CHECKREG r3, 0x15678911;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x67890015;
//CHECKREG r6, 0x74440000;

R5 = ( A0 += A1 );
R6.L = ( A0 += A1 );
R7.H = ( A0 += A1 );
CHECKREG r5, 0x7FFFFFFF;
CHECKREG r6, 0x74447FFF;
CHECKREG r7, 0x7FFF7777;

A0 += A1;
R0 = A0.w;
CHECKREG r0, 0x15678911;

A0 -= A1;
R1 = A0.w;
CHECKREG r1, 0x15678911;

R2 = A1.L + A1.H, R3 = A0.L + A0.H; /* 0x */
CHECKREG r2, 0x00000000;
CHECKREG r3, 0xFFFF9E78;

A0 = A1;
R4 = A0.w;
R5 = A1.w;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;


pass
