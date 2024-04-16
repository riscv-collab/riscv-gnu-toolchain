//Original:/testcases/core/c_dsp32mac_dr_a0_is/c_dsp32mac_dr_a0_is.dsp
// Spec Reference: dsp32mac dr a0 is (scale by 2.0 signed fraction with round)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xf3545abd;
imm32 r1, 0x7fbcfec7;
imm32 r2, 0xc7fff679;
imm32 r3, 0xd0799007;
imm32 r4, 0xefb79f69;
imm32 r5, 0xcd35700b;
imm32 r6, 0xe00c87fd;
imm32 r7, 0xf78e909f;
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L ) (ISS2);
R1 = A0.w;
A1 = R2.L * R3.H, R2.L = ( A0 -= R2.H * R3.L ) (ISS2);
R3 = A0.w;
A1 = R4.H * R5.L, R4.L = ( A0 += R4.H * R5.H ) (ISS2);
R5 = A0.w;
A1 -= R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H ) (ISS2);
R7 = A0.w;
CHECKREG r0, 0xF3548000;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0xC7FF8000;
CHECKREG r3, 0xE71226F2;
CHECKREG r4, 0xEFB78000;
CHECKREG r5, 0xEA4D52D5;
CHECKREG r6, 0xE00C8000;
CHECKREG r7, 0xEE42DC2B;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0xc5548abd;
imm32 r1, 0x9b5cfec7;
imm32 r2, 0xa9b55679;
imm32 r3, 0xb09b5007;
imm32 r4, 0xcfb9b5c9;
imm32 r5, 0x52359b5c;
imm32 r6, 0xe50c5098;
imm32 r7, 0x675e7509;
R0.L = ( A0 -= R1.L * R0.L ) (ISS2);
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H ) (ISS2);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.L ) (ISS2);
R5 = A0.w;
R6.L = ( A0 -= R6.H * R7.H ) (ISS2);
R7 = A0.w;
CHECKREG r0, 0xC5548000;
CHECKREG r1, 0xEDB37D40;
CHECKREG r2, 0xA9B58000;
CHECKREG r3, 0xD2E20883;
CHECKREG r4, 0xCFB97FFF;
CHECKREG r5, 0x12FAA97C;
CHECKREG r6, 0xE50C7FFF;
CHECKREG r7, 0x1DDCBB14;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x4b54babd;
imm32 r1, 0x12346ec7;
imm32 r2, 0xa4bbe679;
imm32 r3, 0x8abdb707;
imm32 r4, 0x9f4b7b69;
imm32 r5, 0xa234877b;
imm32 r6, 0xb00c4887;
imm32 r7, 0xc78ea4b8;
R0.L = ( A0 = R1.L * R0.L ) (ISS2);
R1 = A0.w;
R2.L = ( A0 -= R2.H * R3.L ) (ISS2);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.H ) (ISS2);
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H ) (ISS2);
R7 = A0.w;
CHECKREG r0, 0x4B548000;
CHECKREG r1, 0xE2075EEB;
CHECKREG r2, 0xA4BB8000;
CHECKREG r3, 0xC80330CE;
CHECKREG r4, 0x9F4B7FFF;
CHECKREG r5, 0x236ED13C;
CHECKREG r6, 0xB00C7FFF;
CHECKREG r7, 0x1370FD1E;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x1a545abd;
imm32 r1, 0x42fcfec7;
imm32 r2, 0xc53f5679;
imm32 r3, 0x9c64f007;
imm32 r4, 0xafc7ec69;
imm32 r5, 0xd23c891b;
imm32 r6, 0xc00cc602;
imm32 r7, 0x678edc7e;
A1 = R1.L * R0.L (M), R2.L = ( A0 += R1.L * R0.L ) (ISS2);
R3 = A0.w;
A1 += R2.L * R3.H (M), R6.L = ( A0 = R2.H * R3.L ) (ISS2);
R7 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 -= R4.H * R5.H ) (ISS2);
R5 = A0.w;
A1 = R6.H * R7.H (M), R0.L = ( A0 += R6.L * R7.H ) (ISS2);
R1 = A0.w;
CHECKREG r0, 0x1A548000;
CHECKREG r1, 0xF0477293;
CHECKREG r2, 0xC53F7FFF;
CHECKREG r3, 0x13020C09;
CHECKREG r4, 0xAFC78000;
CHECKREG r5, 0xEEE57293;
CHECKREG r6, 0xC00C8000;
CHECKREG r7, 0xFD3CE337;



pass
