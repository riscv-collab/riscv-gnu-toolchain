//Original:/testcases/core/c_dsp32mac_dr_a0_i/c_dsp32mac_dr_a0_i.dsp
// Spec Reference: dsp32mac dr a0 i (signed int)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xa3545abd;
imm32 r1, 0x9dbcfec7;
imm32 r2, 0xc9248679;
imm32 r3, 0xd0969007;
imm32 r4, 0xefb94569;
imm32 r5, 0xcd35900b;
imm32 r6, 0xe00c890d;
imm32 r7, 0xf78e909f;
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L ) (IS);
R1 = A0.w;
A1 -= R2.L * R3.H, R2.L = ( A0 = R2.H * R3.L ) (IS);
R3 = A0.w;
A1 -= R4.H * R5.L, R4.L = ( A0 += R4.H * R5.H ) (IS);
R5 = A0.w;
A1 += R6.H * R7.H, R6.L = ( A0 -= R6.L * R7.H ) (IS);
R7 = A0.w;
CHECKREG r0, 0xA3548000;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0xC9247FFF;
CHECKREG r3, 0x17FEBFFC;
CHECKREG r4, 0xEFB97FFF;
CHECKREG r5, 0x1B398649;
CHECKREG r6, 0xE00C7FFF;
CHECKREG r7, 0x174CF613;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x68548abd;
imm32 r1, 0x7d8cfec7;
imm32 r2, 0xa1285679;
imm32 r3, 0xb0068007;
imm32 r4, 0xcfbc4869;
imm32 r5, 0xd235c08b;
imm32 r6, 0xe00ca008;
imm32 r7, 0x678e700f;
R0.L = ( A0 -= R1.L * R0.L ) (IS);
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H ) (IS);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.L ) (IS);
R5 = A0.w;
R6.L = ( A0 -= R6.H * R7.H ) (IS);
R7 = A0.w;
CHECKREG r0, 0x68547FFF;
CHECKREG r1, 0x16BD9728;
CHECKREG r2, 0xA1288000;
CHECKREG r3, 0xFBB9CDFE;
CHECKREG r4, 0xCFBC7FFF;
CHECKREG r5, 0x0BF6CB14;
CHECKREG r6, 0xE00C7FFF;
CHECKREG r7, 0x18E3B06C;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x7b54babd;
imm32 r1, 0xb7bcdec7;
imm32 r2, 0x7b7be679;
imm32 r3, 0x80b77007;
imm32 r4, 0x9fbb7569;
imm32 r5, 0xa235b70b;
imm32 r6, 0xb00c3b7d;
imm32 r7, 0xc78ea0b7;
R0.L = ( A0 = R1.L * R0.L ) (IS);
R1 = A0.w;
R2.L = ( A0 -= R2.H * R3.L ) (IS);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.H ) (IS);
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H ) (IS);
R7 = A0.w;
CHECKREG r0, 0x7B547FFF;
CHECKREG r1, 0x08FD0EEB;
CHECKREG r2, 0x7B7B8000;
CHECKREG r3, 0xD2F3DE8E;
CHECKREG r4, 0x9FBB7FFF;
CHECKREG r5, 0x234567B7;
CHECKREG r6, 0xB00C7FFF;
CHECKREG r7, 0x1627920D;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xe3545abd;
imm32 r1, 0x5ebcfec7;
imm32 r2, 0x71e45679;
imm32 r3, 0x900e0007;
imm32 r4, 0xafbce569;
imm32 r5, 0xd2359e0b;
imm32 r6, 0xc00ca0ed;
imm32 r7, 0x678ed00e;
A1 -= R1.L * R0.L (M), R2.L = ( A0 += R1.L * R0.L ) (IS);
R3 = A0.w;
A1 += R2.L * R3.H (M), R6.L = ( A0 -= R2.H * R3.L ) (IS);
R7 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 = R4.H * R5.H ) (IS);
R5 = A0.w;
A1 = R6.H * R7.H (M), R0.L = ( A0 += R6.L * R7.H ) (IS);
R1 = A0.w;
CHECKREG r0, 0xE3547FFF;
CHECKREG r1, 0x2E5AD9ED;
CHECKREG r2, 0x71E47FFF;
CHECKREG r3, 0x15B8A0F8;
CHECKREG r4, 0xAFBC7FFF;
CHECKREG r5, 0x0E5B99EC;
CHECKREG r6, 0xC00C7FFF;
CHECKREG r7, 0x3FFFCC18;



pass
