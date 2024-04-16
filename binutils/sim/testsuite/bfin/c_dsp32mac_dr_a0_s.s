//Original:/testcases/core/c_dsp32mac_dr_a0_s/c_dsp32mac_dr_a0_s.dsp
// Spec Reference: dsp32mac dr a0 s (scale by 2.0 signed fraction with round)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x83545abd;
imm32 r1, 0x98bcfec7;
imm32 r2, 0xc9948679;
imm32 r3, 0xd0999007;
imm32 r4, 0xefb99569;
imm32 r5, 0xcd35900b;
imm32 r6, 0xe00c89ad;
imm32 r7, 0xf78e909a;
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L ) (S2RND);
R1 = A0.w;
A1 = R2.L * R3.H, R2.L = ( A0 = R2.H * R3.L ) (S2RND);
R3 = A0.w;
A1 = R4.H * R5.L, R4.L = ( A0 += R4.H * R5.H ) (S2RND);
R5 = A0.w;
A1 = R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H ) (S2RND);
R7 = A0.w;
CHECKREG r0, 0x8354FE44;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xC9945F37;
CHECKREG r3, 0x2F9B8618;
CHECKREG r4, 0xEFB96C22;
CHECKREG r5, 0x361112B2;
CHECKREG r6, 0xE00C7BBF;
CHECKREG r7, 0x3DDFA49E;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0xc8548abd;
imm32 r1, 0x7bccfec7;
imm32 r2, 0xa1bc5679;
imm32 r3, 0xb00bc007;
imm32 r4, 0xcfbcb8c9;
imm32 r5, 0x5235cb8c;
imm32 r6, 0xe50ca0b8;
imm32 r7, 0x675e700b;
R0.L = ( A0 = R1.L * R0.L ) (S2RND);
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H ) (S2RND);
R3 = A0.w;
R4.L = ( A0 -= R4.H * R5.L ) (S2RND);
R5 = A0.w;
R6.L = ( A0 = R6.H * R7.H ) (S2RND);
R7 = A0.w;
CHECKREG r0, 0xC854023D;
CHECKREG r1, 0x011EBDD6;
CHECKREG r2, 0xA1BC9635;
CHECKREG r3, 0xCB1A8C3C;
CHECKREG r4, 0xCFBC8000;
CHECKREG r5, 0xB7532E9C;
CHECKREG r6, 0xE50CD478;
CHECKREG r7, 0xEA3BDCD0;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x7b54babd;
imm32 r1, 0xbabcdec7;
imm32 r2, 0xabbbe679;
imm32 r3, 0x8abdb007;
imm32 r4, 0x9fab7b69;
imm32 r5, 0xa23a87bb;
imm32 r6, 0xb00ca88b;
imm32 r7, 0xc78eaab8;
R0.L = ( A0 = R1.L * R0.L ) (S2RND);
R1 = A0.w;
R2.L = ( A0 -= R2.H * R3.L ) (S2RND);
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.H ) (S2RND);
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H ) (S2RND);
R7 = A0.w;
CHECKREG r0, 0x7B5423F4;
CHECKREG r1, 0x11FA1DD6;
CHECKREG r2, 0xABBBBAA7;
CHECKREG r3, 0xDD53999C;
CHECKREG r4, 0x9FAB7FFF;
CHECKREG r5, 0x4692C57C;
CHECKREG r6, 0xB00C7FFF;
CHECKREG r7, 0x6D23D9B0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xfa545abd;
imm32 r1, 0x5ffcfec7;
imm32 r2, 0xc1ef5679;
imm32 r3, 0x9c0ef007;
imm32 r4, 0xafccec69;
imm32 r5, 0xd23c9e1b;
imm32 r6, 0xc00cc0e2;
imm32 r7, 0x678edc0e;
A1 = R1.L * R0.L (M), R2.L = ( A0 += R1.L * R0.L ) (S2RND);
R3 = A0.w;
A1 += R2.L * R3.H (M), R6.L = ( A0 = R2.H * R3.L ) (S2RND);
R7 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 -= R4.H * R5.H ) (S2RND);
R5 = A0.w;
A1 = R6.H * R7.H (M), R0.L = ( A0 += R6.L * R7.H ) (S2RND);
R1 = A0.w;
CHECKREG r0, 0xFA54CF65;
CHECKREG r1, 0xE7B2ACD4;
CHECKREG r2, 0xC1EF7FFF;
CHECKREG r3, 0x6C45F786;
CHECKREG r4, 0xAFCCCEDE;
CHECKREG r5, 0xE76F2094;
CHECKREG r6, 0xC00C0838;
CHECKREG r7, 0x041C3834;



pass
