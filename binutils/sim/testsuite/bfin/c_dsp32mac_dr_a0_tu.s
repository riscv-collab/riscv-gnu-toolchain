//Original:/testcases/core/c_dsp32mac_dr_a0_tu/c_dsp32mac_dr_a0_tu.dsp
// Spec Reference: dsp32mac dr a0 tu (truncate unsigned fraction)
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
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L ) (TFU);
R1 = A0.w;
A1 -= R2.L * R3.H, R2.L = ( A0 -= R2.H * R3.L ) (TFU);
R3 = A0.w;
A1 += R4.H * R5.L, R4.L = ( A0 -= R4.H * R5.H ) (TFU);
R5 = A0.w;
A1 += R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H ) (TFU);
R7 = A0.w;
CHECKREG r0, 0xF3545A4E;
CHECKREG r1, 0x5A4E0EEB;
CHECKREG r2, 0xC7FF0000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0xEFB70000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0xE00C8380;
CHECKREG r7, 0x83808956;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0xc5548abd;
imm32 r1, 0x9b5cfec7;
imm32 r2, 0xa9b55679;
imm32 r3, 0xb09b5007;
imm32 r4, 0xcfb9b5c9;
imm32 r5, 0x52359b5c;
imm32 r6, 0xe50c5098;
imm32 r7, 0x675e7509;
R0.L = ( A0 = R1.L * R0.L ) (TFU);
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H ) (TFU);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.L ) (TFU);
R5 = A0.w;
R6.L = ( A0 -= R6.H * R7.H ) (TFU);
R7 = A0.w;
CHECKREG r0, 0xC5548A13;
CHECKREG r1, 0x8A135EEB;
CHECKREG r2, 0xA9B5C5BA;
CHECKREG r3, 0xC5BAEA2E;
CHECKREG r4, 0xCFB97E0F;
CHECKREG r5, 0x7E0FA97C;
CHECKREG r6, 0xE50C2193;
CHECKREG r7, 0x2193BB14;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x4b54babd;
imm32 r1, 0x12346ec7;
imm32 r2, 0xa4bbe679;
imm32 r3, 0x8abdb707;
imm32 r4, 0x9f4b7b69;
imm32 r5, 0xa234877b;
imm32 r6, 0xb00c4887;
imm32 r7, 0xc78ea4b8;
R0.L = ( A0 -= R1.L * R0.L ) (TFU);
R1 = A0.w;
R2.L = ( A0 = R2.H * R3.L ) (TFU);
R3 = A0.w;
R4.L = ( A0 -= R4.H * R5.H ) (TFU);
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H ) (TFU);
R7 = A0.w;
CHECKREG r0, 0x4B540000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0xA4BB75C6;
CHECKREG r3, 0x75C62E1D;
CHECKREG r4, 0x9F4B10D8;
CHECKREG r5, 0x10D85CE1;
CHECKREG r6, 0xB00C4961;
CHECKREG r7, 0x496188C3;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x1a545abd;
imm32 r1, 0x42fcfec7;
imm32 r2, 0xc53f5679;
imm32 r3, 0x9c64f007;
imm32 r4, 0xafc7ec69;
imm32 r5, 0xd23c891b;
imm32 r6, 0xc00cc602;
imm32 r7, 0x678edc7e;
A1 -= R1.L * R0.L (M), R2.L = ( A0 += R1.L * R0.L ) (TFU);
R3 = A0.w;
A1 += R2.L * R3.H (M), R6.L = ( A0 -= R2.H * R3.L ) (TFU);
R7 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 = R4.H * R5.H ) (TFU);
R5 = A0.w;
A1 -= R6.H * R7.H (M), R0.L = ( A0 += R6.L * R7.H ) (TFU);
R1 = A0.w;
CHECKREG r0, 0x1A5498EA;
CHECKREG r1, 0x98EA3745;
CHECKREG r2, 0xC53FA3AF;
CHECKREG r3, 0xA3AF97AE;
CHECKREG r4, 0xAFC7905A;
CHECKREG r5, 0x905A70A4;
CHECKREG r6, 0xC00C2ED1;
CHECKREG r7, 0x2ED15DDC;



pass
