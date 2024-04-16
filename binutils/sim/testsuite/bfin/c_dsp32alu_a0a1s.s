//Original:/testcases/core/c_dsp32alu_a0a1s/c_dsp32alu_a0a1s.dsp
// Spec Reference: dsp32alu a0a1s
# mach: bfin

.include "testutils.inc"
	start



A1 = A0 = 0;

imm32 r0, 0x15678911;
imm32 r1, 0xa789ab1d;
imm32 r2, 0xd4445515;
imm32 r3, 0xf6667717;
imm32 r4, 0xe567891b;
imm32 r5, 0x6789ab1d;
imm32 r6, 0xb4445515;
imm32 r7, 0x86667777;
// A0 & A1 types
A0 = R0;
A1 = R1;
 R6 = A0.w;
 R7 = A1.w;
A0 = 0;
A1 = 0;
 R0 = A0.w;
 R1 = A1.w;
A0 = R2;
A1 = R3;
A0 = A0 (S);
A1 = A1 (S);
 R4 = A0.w;
 R5 = A1.w;
A0 = A1;
 R2 = A0.w;
A0 = R3;
A1 = A0;
 R3 = A1.w;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0xF6667717;
CHECKREG r3, 0xF6667717;
CHECKREG r4, 0xD4445515;
CHECKREG r5, 0xF6667717;
CHECKREG r6, 0x15678911;
CHECKREG r7, 0xA789AB1D;

A1 = A0 = 0;
 R0 = A0.w;
 R1 = A1.w;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;

imm32 r0, 0xa1567891;
imm32 r1, 0xba789abd;
imm32 r2, 0xcd412355;
imm32 r3, 0xdf646777;
imm32 r4, 0xe567891b;
imm32 r5, 0x6789ab1d;
imm32 r6, 0xb4445515;
imm32 r7, 0xf666aeb7;

A0 = R4;
A1 = R5;
 R0 = A0.w;
 R1 = A1.w;
A0 = R6;
A1 = R7;
 R2 = A0.w;
 R3 = A1.w;
CHECKREG r0, 0xE567891B;
CHECKREG r1, 0x6789AB1D;
CHECKREG r2, 0xB4445515;
CHECKREG r3, 0xF666AEB7;
CHECKREG r4, 0xE567891B;
CHECKREG r5, 0x6789AB1D;
CHECKREG r6, 0xB4445515;
CHECKREG r7, 0xF666AEB7;


pass
