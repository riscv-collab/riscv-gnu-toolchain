//Original:/testcases/core/c_dsp32mac_dr_a1_i/c_dsp32mac_dr_a1_i.dsp
// Spec Reference: dsp32mac dr a1 i (signed int)
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
R0.H = ( A1 = R1.L * R0.L ), A0 = R1.L * R0.L (IS);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ), A0 = R2.H * R3.L (IS);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ), A0 += R4.H * R5.H (IS);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ), A0 += R6.L * R7.H (IS);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x7FFF8679;
CHECKREG r3, 0x16C676D6;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0xFAEA0D14;
CHECKREG r6, 0x7FFF800D;
CHECKREG r7, 0x010DDAA8;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x63548abd;
imm32 r1, 0x7dbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0xb0069007;
imm32 r4, 0xcfbc4569;
imm32 r5, 0xFFFF8000;
imm32 r6, 0x7FFF800D;
imm32 r7, 0x00007FFF;
R0.H = ( A1 = R1.L * R0.L ) (IS);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (IS);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (IS);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (IS);
R7 = A1.w;
CHECKREG r0, 0x7FFF8ABD;
CHECKREG r1, 0x008F5EEB;
CHECKREG r2, 0x80005679;
CHECKREG r3, 0xE58B95C1;
CHECKREG r4, 0x7FFF4569;
CHECKREG r5, 0x18220000;
CHECKREG r6, 0x0000800D;
CHECKREG r7, 0x00000000;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x5354babd;
imm32 r1, 0x6dbcdec7;
imm32 r2, 0x7124e679;
imm32 r3, 0x80067007;
imm32 r4, 0x9fbc4569;
imm32 r5, 0xa235900b;
imm32 r6, 0xb00c300d;
imm32 r7, 0xc78ea00f;
 R0.H = A1 , A0 = R1.L * R0.L (IS);
R1 = A1.w;
 R2.H = A1 , A0 = R2.H * R3.L (IS);
R3 = A1.w;
 R4.H = A1 , A0 = R4.H * R5.H (IS);
R5 = A1.w;
 R6.H = A1 , A0 += R6.L * R7.H (IS);
R7 = A1.w;
CHECKREG r0, 0x0000BABD;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x0000E679;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00004569;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x0000300D;
CHECKREG r7, 0x00000000;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 += R1.L * R0.L (IS);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M), A0 = R2.H * R3.L (IS);
R3 = A0.w;
R4.H = ( A1 += R4.H * R5.L ) (M), A0 = R4.H * R5.H (IS);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M), A0 += R6.L * R7.H (IS);
R7 = A0.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x7FFF5679;
CHECKREG r3, 0x000317FC;
CHECKREG r4, 0x7FFF4569;
CHECKREG r5, 0x030D72D5;
CHECKREG r6, 0x8000A00D;
CHECKREG r7, 0xE78B9C22;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83545abd;
imm32 r1, 0xa8bcfec7;
imm32 r2, 0xc1845679;
imm32 r3, 0x1c080007;
imm32 r4, 0xe1cc8569;
imm32 r5, 0x921c080b;
imm32 r6, 0x7901908d;
imm32 r7, 0x679e9008;
R0.H = ( A1 += R1.L * R0.L ) (M,IS);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ) (M,IS);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ) (M,IS);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M,IS);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xE5B26993;
CHECKREG r2, 0x7FFF5679;
CHECKREG r3, 0x0977EFC8;
CHECKREG r4, 0x7FFF8569;
CHECKREG r5, 0x0885038C;
CHECKREG r6, 0x7FFF908D;
CHECKREG r7, 0x30FA159E;

imm32 r0, 0x03545abd;
imm32 r1, 0x1dbcfec7;
imm32 r2, 0x21248679;
imm32 r3, 0x30069007;
imm32 r4, 0x4fbc4569;
imm32 r5, 0x5d35500b;
imm32 r6, 0x600c800d;
imm32 r7, 0x778e900f;
R0.H = ( A1 -= R1.L * R0.L ), A0 = R1.L * R0.L (IS);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ), A0 -= R2.H * R3.L (IS);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ), A0 += R4.H * R5.H (IS);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ), A0 -= R6.L * R7.H (IS);
R7 = A1.w;
CHECKREG r0, 0x7FFF5ABD;
CHECKREG r1, 0x316906B3;
CHECKREG r2, 0x80008679;
CHECKREG r3, 0xE933D6D6;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0xD045A9C2;
CHECKREG r6, 0x8000800D;
CHECKREG r7, 0xA36ACF1A;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x63540abd;
imm32 r1, 0x7dbc1ec7;
imm32 r2, 0xa1242679;
imm32 r3, 0x40063007;
imm32 r4, 0x1fbc4569;
imm32 r5, 0x2FFF4000;
imm32 r6, 0x7FFF800D;
imm32 r7, 0x10007FFF;
R0.H = ( A1 -= R1.L * R0.L ) (IS);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (IS);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (IS);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (IS);
R7 = A1.w;
CHECKREG r0, 0x80000ABD;
CHECKREG r1, 0xA220502F;
CHECKREG r2, 0x80002679;
CHECKREG r3, 0x98812959;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0x90922959;
CHECKREG r6, 0x8000800D;
CHECKREG r7, 0x88923959;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x2354babd;
imm32 r1, 0x3dbcdec7;
imm32 r2, 0x7424e679;
imm32 r3, 0x80067007;
imm32 r4, 0x95bc4569;
imm32 r5, 0xa235900b;
imm32 r6, 0xb06c300d;
imm32 r7, 0xc787a00f;
 R0.H = A1 , A0 -= R1.L * R0.L (IS);
R1 = A1.w;
 R2.H = A1 , A0 -= R2.H * R3.L (IS);
R3 = A1.w;
 R4.H = A1 , A0 -= R4.H * R5.H (IS);
R5 = A1.w;
 R6.H = A1 , A0 -= R6.L * R7.H (IS);
R7 = A1.w;
CHECKREG r0, 0x8000BABD;
CHECKREG r1, 0x88923959;
CHECKREG r2, 0x8000E679;
CHECKREG r3, 0x88923959;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0x88923959;
CHECKREG r6, 0x8000300D;
CHECKREG r7, 0x88923959;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
R0.H = ( A1 -= R1.L * R0.L ) (M), A0 += R1.L * R0.L (IS);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M), A0 = R2.H * R3.L (IS);
R3 = A0.w;
R4.H = ( A1 += R4.H * R5.L ) (M), A0 -= R4.H * R5.H (IS);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (M), A0 += R6.L * R7.H (IS);
R7 = A0.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0x89012A6E;
CHECKREG r2, 0x80005679;
CHECKREG r3, 0x000317FC;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0x2B3160AC;
CHECKREG r6, 0x8000A00D;
CHECKREG r7, 0xCAD78046;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83545abd;
imm32 r1, 0xa8bcfec7;
imm32 r2, 0xc1845679;
imm32 r3, 0x1c080007;
imm32 r4, 0xe1cc8569;
imm32 r5, 0x921c080b;
imm32 r6, 0x7901908d;
imm32 r7, 0x679e9008;
R0.H = ( A1 -= R1.L * R0.L ) (M,IS);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M,IS);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (M,IS);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (M,IS);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0x457EF719;
CHECKREG r2, 0x80005679;
CHECKREG r3, 0x3C070751;
CHECKREG r4, 0x80008569;
CHECKREG r5, 0x3CF9F38D;
CHECKREG r6, 0x8000908D;
CHECKREG r7, 0x0BFFDDEF;



pass
