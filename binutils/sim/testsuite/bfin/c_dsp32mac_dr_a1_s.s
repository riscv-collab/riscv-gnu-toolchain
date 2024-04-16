//Original:/testcases/core/c_dsp32mac_dr_a1_s/c_dsp32mac_dr_a1_s.dsp
// Spec Reference: dsp32mac dr_a1 s (scale by 2 signed fraction with round)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0xa3545abd;
imm32 r1, 0xbabcfec7;
imm32 r2, 0xc1a48679;
imm32 r3, 0xd00a9007;
imm32 r4, 0xefbca569;
imm32 r5, 0xcd355a0b;
imm32 r6, 0xe00c80ad;
imm32 r7, 0xf78e900a;
R0.H = ( A1 -= R1.L * R0.L ), A0 += R1.L * R0.L (S2RND);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ), A0 -= R2.H * R3.L (S2RND);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ), A0 = R4.H * R5.H (S2RND);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ), A0 -= R6.L * R7.H (S2RND);
R7 = A1.w;
CHECKREG r0, 0x01BC5ABD;
CHECKREG r1, 0x00DDE22A;
CHECKREG r2, 0x5CCE8679;
CHECKREG r3, 0x2E67039E;
CHECKREG r4, 0xE91EA569;
CHECKREG r5, 0xF48ECA28;
CHECKREG r6, 0xED5580AD;
CHECKREG r7, 0xF6AA7F78;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0x63bb8abd;
imm32 r1, 0xbdbcfec7;
imm32 r2, 0xab245679;
imm32 r3, 0xb0b69007;
imm32 r4, 0xcfbb4569;
imm32 r5, 0xd235b00b;
imm32 r6, 0xe00cab0d;
imm32 r7, 0x678e70bf;
R0.H = ( A1 += R1.L * R0.L ) (S2RND);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (S2RND);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ) (S2RND);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (S2RND);
R7 = A1.w;
CHECKREG r0, 0xEF928ABD;
CHECKREG r1, 0xF7C93D4E;
CHECKREG r2, 0x5AB45679;
CHECKREG r3, 0x2D59E942;
CHECKREG r4, 0x7FFF4569;
CHECKREG r5, 0x4B80E354;
CHECKREG r6, 0xCC4CAB0D;
CHECKREG r7, 0xE6263550;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0x5c54babd;
imm32 r1, 0x6dccdec7;
imm32 r2, 0xc12ce679;
imm32 r3, 0x8c06c007;
imm32 r4, 0x9fcc4c69;
imm32 r5, 0xa23c90cb;
imm32 r6, 0xb00cc00c;
imm32 r7, 0xc78eac0f;
 R0.H = A1 , A0 -= R1.L * R0.L (S2RND);
R1 = A1.w;
 R2.H = A1 , A0 += R2.H * R3.L (S2RND);
R3 = A1.w;
 R4.H = A1 , A0 = R4.H * R5.H (S2RND);
R5 = A1.w;
 R6.H = A1 , A0 += R6.L * R7.H (S2RND);
R7 = A1.w;
CHECKREG r0, 0xCC4CBABD;
CHECKREG r1, 0xE6263550;
CHECKREG r2, 0xCC4CE679;
CHECKREG r3, 0xE6263550;
CHECKREG r4, 0xCC4C4C69;
CHECKREG r5, 0xE6263550;
CHECKREG r6, 0xCC4CC00C;
CHECKREG r7, 0xE6263550;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x3d545abd;
imm32 r1, 0x5ddcfec7;
imm32 r2, 0x712d5679;
imm32 r3, 0x9006d007;
imm32 r4, 0xafbc4d69;
imm32 r5, 0xd23590db;
imm32 r6, 0xd00ca00d;
imm32 r7, 0x6d8ed00f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 += R1.L * R0.L (S2RND);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ) (M), A0 -= R2.H * R3.L (S2RND);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ) (M), A0 = R4.H * R5.H (S2RND);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ) (M), A0 += R6.L * R7.H (S2RND);
R7 = A1.w;
CHECKREG r0, 0xFF225ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x614C5679;
CHECKREG r3, 0x30A616D6;
CHECKREG r4, 0x06764D69;
CHECKREG r5, 0x033B2CAA;
CHECKREG r6, 0xDD6BA00D;
CHECKREG r7, 0xEEB5AF52;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x83e45abd;
imm32 r1, 0xe8befec7;
imm32 r2, 0xce84e679;
imm32 r3, 0x1ce80e07;
imm32 r4, 0xe1ce85e9;
imm32 r5, 0x921ce80e;
imm32 r6, 0x79019e8d;
imm32 r7, 0x679e90e8;
R0.H = ( A1 += R1.L * R0.L ) (M,S2RND);
R1 = A1.w;
R2.H = ( A1 = R2.L * R3.H ) (M,S2RND);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ) (M,S2RND);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (M,S2RND);
R7 = A1.w;
CHECKREG r0, 0xDC8D5ABD;
CHECKREG r1, 0xEE46BE3D;
CHECKREG r2, 0xFA3CE679;
CHECKREG r3, 0xFD1E19A8;
CHECKREG r4, 0xC37E85E9;
CHECKREG r5, 0xE1BF22EC;
CHECKREG r6, 0x80009E8D;
CHECKREG r7, 0xB0C50D4E;



pass
