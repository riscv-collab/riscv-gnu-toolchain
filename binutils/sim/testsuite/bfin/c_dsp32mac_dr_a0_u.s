//Original:/testcases/core/c_dsp32mac_dr_a0_u/c_dsp32mac_dr_a0_u.dsp
// Spec Reference: dsp32mac dr a0 u (unsigned fraction and unsigned int)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xa3545abd;
imm32 r1, 0x9abcfec7;
imm32 r2, 0xc9a48679;
imm32 r3, 0xd09a9007;
imm32 r4, 0xefb9a569;
imm32 r5, 0xcd359a0b;
imm32 r6, 0xe00c89ad;
imm32 r7, 0xf78e909a;
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L ) (FU);
R1 = A0.w;
A1 = R2.L * R3.H, R2.L = ( A0 -= R2.H * R3.L ) (FU);
R3 = A0.w;
A1 -= R4.H * R5.L, R4.L = ( A0 += R4.H * R5.H ) (FU);
R5 = A0.w;
A1 = R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H ) (FU);
R7 = A0.w;
CHECKREG r0, 0xA3545A4E;
CHECKREG r1, 0x5A4E0EEB;
CHECKREG r2, 0xC9A40000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0xEFB9C029;
CHECKREG r5, 0xC028C64D;
CHECKREG r6, 0xE00CFFFF;
CHECKREG r7, 0x454B0F43;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0xb8548abd;
imm32 r1, 0x7b8cfec7;
imm32 r2, 0xa1b85679;
imm32 r3, 0xb00b8007;
imm32 r4, 0xcfbcb869;
imm32 r5, 0xd235cb8b;
imm32 r6, 0xe00ca0b8;
imm32 r7, 0x678e700b;
R0.L = ( A0 = R1.L * R0.L ) (FU);
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H ) (FU);
R3 = A0.w;
R4.L = ( A0 -= R4.H * R5.L ) (FU);
R5 = A0.w;
R6.L = ( A0 = R6.H * R7.H ) (FU);
R7 = A0.w;
CHECKREG r0, 0xB8548A13;
CHECKREG r1, 0x8A135EEB;
CHECKREG r2, 0xA1B8C58A;
CHECKREG r3, 0xC58A461E;
CHECKREG r4, 0xCFBC205F;
CHECKREG r5, 0x205F670A;
CHECKREG r6, 0xE00C5AA1;
CHECKREG r7, 0x5AA11AA8;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x7b54babd;
imm32 r1, 0xb7bcdec7;
imm32 r2, 0xab7be679;
imm32 r3, 0x8ab7b007;
imm32 r4, 0x9fab7b69;
imm32 r5, 0xa23ab7bb;
imm32 r6, 0xb00cab7b;
imm32 r7, 0xc78eaab7;
R0.L = ( A0 = R1.L * R0.L ) (FU);
R1 = A0.w;
R2.L = ( A0 -= R2.H * R3.L ) (FU);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.H ) (FU);
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H ) (FU);
R7 = A0.w;
CHECKREG r0, 0x7B54A281;
CHECKREG r1, 0xA2810EEB;
CHECKREG r2, 0xAB7B2C98;
CHECKREG r3, 0x2C97CE8E;
CHECKREG r4, 0x9FAB652E;
CHECKREG r5, 0x652E62BE;
CHECKREG r6, 0xB00CEADA;
CHECKREG r7, 0xEADA1DF8;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xea545abd;
imm32 r1, 0x5eacfec7;
imm32 r2, 0xc1ea5679;
imm32 r3, 0x9c0ea007;
imm32 r4, 0xafccea69;
imm32 r5, 0xd23c9eab;
imm32 r6, 0xc00cc0ea;
imm32 r7, 0x678edc0e;
A1 = R1.L * R0.L (M), R2.L = ( A0 += R1.L * R0.L ) (FU);
R3 = A0.w;
A1 += R2.L * R3.H (M), R6.L = ( A0 = R2.H * R3.L ) (FU);
R7 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 -= R4.H * R5.H ) (FU);
R5 = A0.w;
A1 = R6.H * R7.H (M), R0.L = ( A0 += R6.L * R7.H ) (FU);
R1 = A0.w;
CHECKREG r0, 0xEA540484;
CHECKREG r1, 0x04840000;
CHECKREG r2, 0xC1EAFFFF;
CHECKREG r3, 0x45282CE3;
CHECKREG r4, 0xAFCC0000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0xC00C2200;
CHECKREG r7, 0x22002A7E;



pass
