//Original:/testcases/core/c_dsp32mac_dr_a1_iu/c_dsp32mac_dr_a1_iu.dsp
// Spec Reference: dsp32mac dr_a1 iu (unsigned integer)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x93545abd;
imm32 r1, 0x7890afc7;
imm32 r2, 0x52248679;
imm32 r3, 0xd5069007;
imm32 r4, 0xef5c4569;
imm32 r5, 0xcd35500b;
imm32 r6, 0xe00c500d;
imm32 r7, 0xf78e950f;
R0.H = ( A1 = R1.L * R0.L ), A0 += R1.L * R0.L (IU);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ), A0 = R2.H * R3.L (IU);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ), A0 += R4.H * R5.H (IU);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ), A0 -= R6.L * R7.H (IU);
R7 = A1.w;
CHECKREG r0, 0xFFFF5ABD;
CHECKREG r1, 0x3E4DBBEB;
CHECKREG r2, 0xFFFF8679;
CHECKREG r3, 0xAE338FC1;
CHECKREG r4, 0xFFFF4569;
CHECKREG r5, 0xF90A98B5;
CHECKREG r6, 0xFFFF500D;
CHECKREG r7, 0x2062BE0D;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0xd3548abd;
imm32 r1, 0x9dbcfec7;
imm32 r2, 0xa9d45679;
imm32 r3, 0xb09d9007;
imm32 r4, 0xcfb9d569;
imm32 r5, 0xd2359d0b;
imm32 r6, 0xe00ca90d;
imm32 r7, 0x678e709f;
R0.H = ( A1 += R1.L * R0.L ) (IU);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (IU);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (IU);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (IU);
R7 = A1.w;
CHECKREG r0, 0xFFFF8ABD;
CHECKREG r1, 0xAA761CF8;
CHECKREG r2, 0xFFFF5679;
CHECKREG r3, 0x6ECDE4C3;
CHECKREG r4, 0xFFFFD569;
CHECKREG r5, 0x7F6D61F3;
CHECKREG r6, 0xFFFFA90D;
CHECKREG r7, 0x24CC474B;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0xa354babd;
imm32 r1, 0x9abcdec7;
imm32 r2, 0x77a4e679;
imm32 r3, 0x805a7007;
imm32 r4, 0x9fb3a569;
imm32 r5, 0xa2352a0b;
imm32 r6, 0xb00c10ad;
imm32 r7, 0x9876a10a;
 R0.H = A1 , A0 -= R1.L * R0.L (IU);
R1 = A1.w;
 R2.H = A1 , A0 += R2.H * R3.L (IU);
R3 = A1.w;
 R4.H = A1 , A0 = R4.H * R5.H (IU);
R5 = A1.w;
 R6.H = A1 , A0 -= R6.L * R7.H (IU);
R7 = A1.w;
CHECKREG r0, 0xFFFFBABD;
CHECKREG r1, 0x24CC474B;
CHECKREG r2, 0xFFFFE679;
CHECKREG r3, 0x24CC474B;
CHECKREG r4, 0xFFFFA569;
CHECKREG r5, 0x24CC474B;
CHECKREG r6, 0xFFFF10AD;
CHECKREG r7, 0x24CC474B;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x9dbcfec7;
imm32 r2, 0x81245679;
imm32 r3, 0x97060007;
imm32 r4, 0xaf6c4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00c400d;
imm32 r7, 0x678ed30f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 = R1.L * R0.L (IU);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M), A0 = R2.H * R3.L (IU);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (M), A0 -= R4.H * R5.H (IU);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ) (M), A0 -= R6.L * R7.H (IU);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x80005679;
CHECKREG r3, 0xCC8DA915;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0xD2A949A4;
CHECKREG r6, 0x8000400D;
CHECKREG r7, 0xB8CAA44C;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0xe2005ABD;
imm32 r1, 0x0e300000;
imm32 r2, 0x56e49679;
imm32 r3, 0x30Ae5000;
imm32 r4, 0xa000e669;
imm32 r5, 0x01000e70;
imm32 r6, 0xdf4560eD;
imm32 r7, 0x1234567e;
R0.H = ( A1 -= R1.L * R0.L ) (M,IU);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M,IU);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (M,IU);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (M,IU);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xB8CAA44C;
CHECKREG r2, 0x80009679;
CHECKREG r3, 0xA4B99A8A;
CHECKREG r4, 0x8000E669;
CHECKREG r5, 0xAA239A8A;
CHECKREG r6, 0x800060ED;
CHECKREG r7, 0xAC776686;



pass
