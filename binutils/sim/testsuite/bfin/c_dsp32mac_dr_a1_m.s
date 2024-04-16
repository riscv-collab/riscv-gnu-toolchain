//Original:/testcases/core/c_dsp32mac_dr_a1_m/c_dsp32mac_dr_a1_m.dsp
// Spec Reference: dsp32mac dr a1 m
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0xab235675;
imm32 r1, 0xcfba5127;
imm32 r2, 0x13246705;
imm32 r3, 0x00060007;
imm32 r4, 0x90abcd09;
imm32 r5, 0x10acefdb;
imm32 r6, 0x000c000d;
imm32 r7, 0x1246700f;

A1 = A0 = 0;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
R0.H = ( A1 += R1.L * R0.L ), A0 = R1.L * R0.L;
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ), A0 = R2.H * R3.L;
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ), A0 += R4.H * R5.H;
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ), A0 += R6.L * R7.H;
R7 = A1.w;
CHECKREG r0, 0xFF225ABD;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0x00045679;
CHECKREG r3, 0x00040DAC;
CHECKREG r4, 0xFFFF4569;
CHECKREG r5, 0xFFFE9A28;
CHECKREG r6, 0x0008000D;
CHECKREG r7, 0x00084F78;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
R0.H = ( A1 += R1.L * R0.L );
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H );
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L );
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H );
R7 = A1.w;
CHECKREG r0, 0xFF2A5ABD;
CHECKREG r1, 0xFF2A6D4E;
CHECKREG r2, 0x00045679;
CHECKREG r3, 0x00040DAC;
CHECKREG r4, 0x00034569;
CHECKREG r5, 0x0002A7D4;
CHECKREG r6, 0x000A000D;
CHECKREG r7, 0x0009B550;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
 R0.H = A1 , A0 += R1.L * R0.L;
R1 = A1.w;
 R2.H = A1 , A0 = R2.H * R3.L;
R3 = A1.w;
 R4.H = A1 , A0 = R4.H * R5.H;
R5 = A1.w;
 R6.H = A1 , A0 += R6.L * R7.H;
R7 = A1.w;
CHECKREG r0, 0x000A5ABD;
CHECKREG r1, 0x0009B550;
CHECKREG r2, 0x000A5679;
CHECKREG r3, 0x0009B550;
CHECKREG r4, 0x000A4569;
CHECKREG r5, 0x0009B550;
CHECKREG r6, 0x000A000D;
CHECKREG r7, 0x0009B550;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
R4.H = ( A1 += R1.L * R0.L ) (M), A0 = R1.L * R0.L;
R5 = A1.w;
R6.H = ( A1 = R2.L * R3.H ) (M), A0 += R2.H * R3.L;
R7 = A1.w;
R0.H = ( A1 = R4.H * R5.L ) (M), A0 = R4.H * R5.H;
R1 = A1.w;
R2.H = ( A1 = R6.H * R7.H ) (M), A0 += R6.L * R7.H;
R3 = A1.w;
CHECKREG r0, 0xFFB35ABD;
CHECKREG r1, 0xFFB294B9;
CHECKREG r2, 0x00005679;
CHECKREG r3, 0x00000004;
CHECKREG r4, 0xFF9B4569;
CHECKREG r5, 0xFF9AC43B;
CHECKREG r6, 0x0002000D;

CHECKREG r7, 0x000206D6;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83545abd;
imm32 r1, 0xa8bcfec7;
imm32 r2, 0xc1845679;
imm32 r3, 0x1c080007;
imm32 r4, 0xe1cc8569;
imm32 r5, 0x121c080b;
imm32 r6, 0x7001008d;
imm32 r7, 0x678e1008;
R6.H = ( A1 += R1.L * R0.L ) (M);
R7 = A1.w;
R2.H = ( A1 = R2.L * R3.H ) (M);
R3 = A1.w;
R0.H = ( A1 += R4.H * R5.L ) (M);
R1 = A1.w;
R4.H = ( A1 = R6.H * R7.H ) (M);
R5 = A1.w;
CHECKREG r0, 0x08855ABD;
CHECKREG r1, 0x0885038C;
CHECKREG r2, 0x09785679;
CHECKREG r3, 0x0977EFC8;
CHECKREG r4, 0xFF918569;
CHECKREG r5, 0xFF913021;
CHECKREG r6, 0xFF91008D;
CHECKREG r7, 0xFF910EEF;

imm32 r0, 0x03545abd;
imm32 r1, 0xa0bcfec7;
imm32 r2, 0xa1045679;
imm32 r3, 0x00000007;
imm32 r4, 0xefbc0569;
imm32 r5, 0x1235100b;
imm32 r6, 0x000c020d;
imm32 r7, 0x678e003f;
R4.H = ( A1 -= R1.L * R0.L ) (M), A0 -= R1.L * R0.L;
R5 = A1.w;
R6.H = ( A1 -= R2.L * R3.H ) (M), A0 += R2.H * R3.L;
R7 = A1.w;
R0.H = ( A1 += R4.H * R5.L ) (M), A0 -= R4.H * R5.H;
R1 = A1.w;
R2.H = ( A1 -= R6.H * R7.H ) (M), A0 -= R6.L * R7.H;
R3 = A1.w;
CHECKREG r0, 0x00005ABD;
CHECKREG r1, 0x00002136;
CHECKREG r2, 0x00005679;
CHECKREG r3, 0x00002136;
CHECKREG r4, 0x00000569;
CHECKREG r5, 0x00002136;
CHECKREG r6, 0x0000020D;
CHECKREG r7, 0x00002136;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83545abd;
imm32 r1, 0xa8bcfec7;
imm32 r2, 0xc1845679;
imm32 r3, 0x1c080007;
imm32 r4, 0xe1cc8569;
imm32 r5, 0x121c080b;
imm32 r6, 0x7001008d;
imm32 r7, 0x678e1008;
R6.H = ( A1 -= R1.L * R0.L ) (M);
R7 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M);
R3 = A1.w;
R0.H = ( A1 -= R4.H * R5.L ) (M);
R1 = A1.w;
R4.H = ( A1 -= R6.H * R7.H ) (M);
R5 = A1.w;
CHECKREG r0, 0xF7EA5ABD;
CHECKREG r1, 0xF7EA0EBF;
CHECKREG r2, 0xF6F75679;
CHECKREG r3, 0xF6F72283;
CHECKREG r4, 0xF7EA8569;
CHECKREG r5, 0xF7E9DE9E;
CHECKREG r6, 0x006F008D;
CHECKREG r7, 0x006F124B;



pass
