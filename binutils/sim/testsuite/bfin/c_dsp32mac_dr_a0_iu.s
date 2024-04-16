//Original:/testcases/core/c_dsp32mac_dr_a0_iu/c_dsp32mac_dr_a0_iu.dsp
// Spec Reference: dsp32mac dr a0 iu (unsigned int)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x83545abd;
imm32 r1, 0x78bcfec7;
imm32 r2, 0xc7948679;
imm32 r3, 0xd0799007;
imm32 r4, 0xefb79569;
imm32 r5, 0xcd35700b;
imm32 r6, 0xe00c877d;
imm32 r7, 0xf78e9097;
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
A1 -= R2.L * R3.H, R2.L = ( A0 = R2.H * R3.L );
R3 = A0.w;
A1 = R4.H * R5.L, R4.L = ( A0 -= R4.H * R5.H );
R5 = A0.w;
A1 -= R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0x8354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xC794315B;
CHECKREG r3, 0x315B6A18;
CHECKREG r4, 0xEFB72AE5;
CHECKREG r5, 0x2AE51252;
CHECKREG r6, 0xE00C32D9;
CHECKREG r7, 0x32D896FE;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0xc5548abd;
imm32 r1, 0x7b5cfec7;
imm32 r2, 0xa1b55679;
imm32 r3, 0xb00b5007;
imm32 r4, 0xcfbcb5c9;
imm32 r5, 0x5235cb5c;
imm32 r6, 0xe50c50b8;
imm32 r7, 0x675e750b;
R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H );
R3 = A0.w;
R4.L = ( A0 -= R4.H * R5.L );
R5 = A0.w;
R6.L = ( A0 = R6.H * R7.H );
R7 = A0.w;
CHECKREG r0, 0xC554011F;
CHECKREG r1, 0x011EBDD6;
CHECKREG r2, 0xA1B5CB1B;
CHECKREG r3, 0xCB1A8C3C;
CHECKREG r4, 0xCFBCB741;
CHECKREG r5, 0xB741151C;
CHECKREG r6, 0xE50CEA3C;
CHECKREG r7, 0xEA3BDCD0;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x4b54babd;
imm32 r1, 0xbabcdec7;
imm32 r2, 0xa4bbe679;
imm32 r3, 0x8abdb007;
imm32 r4, 0x9f4b7b69;
imm32 r5, 0xa23487bb;
imm32 r6, 0xb00c488b;
imm32 r7, 0xc78ea4b8;
R0.L = ( A0 -= R1.L * R0.L );
R1 = A0.w;
R2.L = ( A0 = R2.H * R3.L );
R3 = A0.w;
R4.L = ( A0 = R4.H * R5.H );
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0x4B54D842;
CHECKREG r1, 0xD841BEFA;
CHECKREG r2, 0xA4BB3906;
CHECKREG r3, 0x3906223A;
CHECKREG r4, 0x9F4B46DE;
CHECKREG r5, 0x46DDA278;
CHECKREG r6, 0xB00C26E0;
CHECKREG r7, 0x26E036AC;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x1a545abd;
imm32 r1, 0x52fcfec7;
imm32 r2, 0xc13f5679;
imm32 r3, 0x9c04f007;
imm32 r4, 0xafccec69;
imm32 r5, 0xd23c5e1b;
imm32 r6, 0xc00cc6e2;
imm32 r7, 0x678edc7e;
A1 = R1.L * R0.L (M), R2.L = ( A0 += R1.L * R0.L );
R3 = A0.w;
A1 += R2.L * R3.H (M), R6.L = ( A0 -= R2.H * R3.L );
R7 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 = R4.H * R5.H );
R5 = A0.w;
A1 = R6.H * R7.H (M), R0.L = ( A0 += R6.L * R7.H );
R1 = A0.w;
CHECKREG r0, 0x1A544DFA;
CHECKREG r1, 0x4DFA5880;
CHECKREG r2, 0xC13F2602;
CHECKREG r3, 0x26025482;
CHECKREG r4, 0xAFCC1CAD;
CHECKREG r5, 0x1CAD17A0;
CHECKREG r6, 0xC00C4F71;
CHECKREG r7, 0x4F70B886;



pass
