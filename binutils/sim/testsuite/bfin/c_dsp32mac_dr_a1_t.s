//Original:/testcases/core/c_dsp32mac_dr_a1_t/c_dsp32mac_dr_a1_t.dsp
// Spec Reference: dsp32mac dr a1 t (truncation)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0xa3545abd;
imm32 r1, 0xbdbcfec7;
imm32 r2, 0xc1248679;
imm32 r3, 0xd0069007;
imm32 r4, 0xefbc4569;
imm32 r5, 0xcd35500b;
imm32 r6, 0xe00c800d;
imm32 r7, 0xf78e900f;
R0.H = ( A1 = R1.L * R0.L ), A0 = R1.L * R0.L (T);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ), A0 = R2.H * R3.L (T);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ), A0 += R4.H * R5.H (T);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ), A0 += R6.L * R7.H (T);
R7 = A1.w;
CHECKREG r0, 0xFF225ABD;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0x2D8C8679;
CHECKREG r3, 0x2D8CEDAC;
CHECKREG r4, 0xF5D44569;
CHECKREG r5, 0xF5D41A28;
CHECKREG r6, 0x021B800D;
CHECKREG r7, 0x021BB550;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x63548abd;
imm32 r1, 0x7dbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0xb0069007;
imm32 r4, 0xcfbc4569;
imm32 r5, 0xd235c00b;
imm32 r6, 0xe00ca00d;
imm32 r7, 0x678e700f;
R0.H = ( A1 = R1.L * R0.L ) (T);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (T);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (T);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (T);
R7 = A1.w;
CHECKREG r0, 0x011E8ABD;
CHECKREG r1, 0x011EBDD6;
CHECKREG r2, 0xCB175679;
CHECKREG r3, 0xCB172B82;
CHECKREG r4, 0x181D4569;
CHECKREG r5, 0x181DDA28;
CHECKREG r6, 0xE626A00D;
CHECKREG r7, 0xE6263550;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x5354babd;
imm32 r1, 0x6dbcdec7;
imm32 r2, 0x7124e679;
imm32 r3, 0x80067007;
imm32 r4, 0x9fbc4569;
imm32 r5, 0xa235900b;
imm32 r6, 0xb00c300d;
imm32 r7, 0xc78ea00f;
 R0.H = A1 , A0 = R1.L * R0.L (T);
R1 = A1.w;
 R2.H = A1 , A0 = R2.H * R3.L (T);
R3 = A1.w;
 R4.H = A1 , A0 = R4.H * R5.H (T);
R5 = A1.w;
 R6.H = A1 , A0 += R6.L * R7.H (T);
R7 = A1.w;
CHECKREG r0, 0xE626BABD;
CHECKREG r1, 0xE6263550;
CHECKREG r2, 0xE626E679;
CHECKREG r3, 0xE6263550;
CHECKREG r4, 0xE6264569;
CHECKREG r5, 0xE6263550;
CHECKREG r6, 0xE626300D;
CHECKREG r7, 0xE6263550;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 += R1.L * R0.L (T);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M), A0 = R2.H * R3.L (T);
R3 = A0.w;
R4.H = ( A1 += R4.H * R5.L ) (M), A0 = R4.H * R5.H (T);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M), A0 += R6.L * R7.H (T);
R7 = A0.w;
CHECKREG r0, 0xFF915ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x30375679;
CHECKREG r3, 0x00062FF8;
CHECKREG r4, 0x030D4569;
CHECKREG r5, 0x030D72D5;
CHECKREG r6, 0xE621A00D;
CHECKREG r7, 0xCF173844;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83545abd;
imm32 r1, 0xa8bcfec7;
imm32 r2, 0xc1845679;
imm32 r3, 0x1c080007;
imm32 r4, 0xe1cc8569;
imm32 r5, 0x921c080b;
imm32 r6, 0x7901908d;
imm32 r7, 0x679e9008;
R0.H = ( A1 += R1.L * R0.L ) (M,T);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ) (M,T);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ) (M,T);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M,T);
R7 = A1.w;
CHECKREG r0, 0xE5B25ABD;
CHECKREG r1, 0xE5B26993;
CHECKREG r2, 0x09775679;
CHECKREG r3, 0x0977EFC8;
CHECKREG r4, 0x08858569;
CHECKREG r5, 0x0885038C;
CHECKREG r6, 0x30FA908D;
CHECKREG r7, 0x30FA159E;

imm32 r0, 0x03545abd;
imm32 r1, 0xb0bcfec7;
imm32 r2, 0xc1048679;
imm32 r3, 0xd0009007;
imm32 r4, 0xefbc0569;
imm32 r5, 0xcd35510b;
imm32 r6, 0xe00c802d;
imm32 r7, 0xf78e9003;
R0.H = ( A1 -= R1.L * R0.L ), A0 = R1.L * R0.L (T);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ), A0 -= R2.H * R3.L (T);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ), A0 += R4.H * R5.H (T);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ), A0 -= R6.L * R7.H (T);
R7 = A1.w;
CHECKREG r0, 0x31D75ABD;
CHECKREG r1, 0x31D7F7C8;
CHECKREG r2, 0x2D928679;
CHECKREG r3, 0x2D92A000;
CHECKREG r4, 0x37DF0569;
CHECKREG r5, 0x37DF0DD8;
CHECKREG r6, 0x39FA802D;
CHECKREG r7, 0x39FAC328;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x63548abd;
imm32 r1, 0x7dbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0xb0069007;
imm32 r4, 0xcfbc4569;
imm32 r5, 0xd235c00b;
imm32 r6, 0xe00ca00d;
imm32 r7, 0x678e700f;
R0.H = ( A1 -= R1.L * R0.L ) (T);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (T);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (T);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (T);
R7 = A1.w;
CHECKREG r0, 0x38DC8ABD;
CHECKREG r1, 0x38DC0552;
CHECKREG r2, 0x6EE35679;
CHECKREG r3, 0x6EE397A6;
CHECKREG r4, 0x56C54569;
CHECKREG r5, 0x56C5BD7E;
CHECKREG r6, 0x709FA00D;
CHECKREG r7, 0x709F882E;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x5354babd;
imm32 r1, 0x6dbcdec7;
imm32 r2, 0x7124e679;
imm32 r3, 0x80067007;
imm32 r4, 0x9fbc4569;
imm32 r5, 0xa235900b;
imm32 r6, 0xb00c300d;
imm32 r7, 0xc78ea00f;
 R0.H = A1 , A0 -= R1.L * R0.L (T);
R1 = A1.w;
 R2.H = A1 , A0 -= R2.H * R3.L (T);
R3 = A1.w;
 R4.H = A1 , A0 -= R4.H * R5.H (T);
R5 = A1.w;
 R6.H = A1 , A0 -= R6.L * R7.H (T);
R7 = A1.w;
CHECKREG r0, 0x709FBABD;
CHECKREG r1, 0x709F882E;
CHECKREG r2, 0x709FE679;
CHECKREG r3, 0x709F882E;
CHECKREG r4, 0x709F4569;
CHECKREG r5, 0x709F882E;
CHECKREG r6, 0x709F300D;
CHECKREG r7, 0x709F882E;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
R0.H = ( A1 -= R1.L * R0.L ) (M), A0 += R1.L * R0.L (T);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M), A0 -= R2.H * R3.L (T);
R3 = A0.w;
R4.H = ( A1 += R4.H * R5.L ) (M), A0 -= R4.H * R5.H (T);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ) (M), A0 -= R6.L * R7.H (T);
R7 = A0.w;
CHECKREG r0, 0x710E5ABD;
CHECKREG r1, 0x710E7943;
CHECKREG r2, 0x40685679;
CHECKREG r3, 0x1ED0EB56;
CHECKREG r4, 0x133E4569;
CHECKREG r5, 0x133EAF81;
CHECKREG r6, 0xF960A00D;
CHECKREG r7, 0x4FB9B312;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83545abd;
imm32 r1, 0xa8bcfec7;
imm32 r2, 0xc1845679;
imm32 r3, 0x1c080007;
imm32 r4, 0xe1cc8569;
imm32 r5, 0x921c080b;
imm32 r6, 0x7901908d;
imm32 r7, 0x679e9008;
R0.H = ( A1 -= R1.L * R0.L ) (M,T);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M,T);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (M,T);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (M,T);
R7 = A1.w;
CHECKREG r0, 0xF9CE5ABD;
CHECKREG r1, 0xF9CEFB3E;
CHECKREG r2, 0xF0575679;
CHECKREG r3, 0xF0570B76;
CHECKREG r4, 0xF1498569;
CHECKREG r5, 0xF149F7B2;
CHECKREG r6, 0xC04F908D;
CHECKREG r7, 0xC04FE214;



pass
