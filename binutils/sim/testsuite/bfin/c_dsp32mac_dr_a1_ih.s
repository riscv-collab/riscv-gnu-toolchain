//Original:/testcases/core/c_dsp32mac_dr_a1_ih/c_dsp32mac_dr_a1_ih.dsp
// Spec Reference: dsp32mac dr_a1 ih (int multiplication with word extraction)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x93545abd;
imm32 r1, 0x1dbcfec7;
imm32 r2, 0x52248679;
imm32 r3, 0xd6069007;
imm32 r4, 0xef7c4569;
imm32 r5, 0xcd38500b;
imm32 r6, 0xe00c900d;
imm32 r7, 0xf78e990f;
R0.H = ( A1 = R1.L * R0.L ), A0 -= R1.L * R0.L (IH);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ), A0 -= R2.H * R3.L (IH);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ), A0 += R4.H * R5.H (IH);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ), A0 = R6.L * R7.H (IH);
R7 = A1.w;
CHECKREG r0, 0xFF915ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x137E8679;
CHECKREG r3, 0x137E5BC1;
CHECKREG r4, 0x18A84569;
CHECKREG r5, 0x18A8516D;
CHECKREG r6, 0x010E900D;
CHECKREG r7, 0x010DDAA8;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0x83548abd;
imm32 r1, 0x76bcfec7;
imm32 r2, 0xa1745679;
imm32 r3, 0xb0269007;
imm32 r4, 0xcfb34569;
imm32 r5, 0xd235600b;
imm32 r6, 0xe00ca70d;
imm32 r7, 0x678e708f;
R0.H = ( A1 -= R1.L * R0.L ) (IH);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (IH);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (IH);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (IH);
R7 = A1.w;
CHECKREG r0, 0x007E8ABD;
CHECKREG r1, 0x007E7BBD;
CHECKREG r2, 0xE5865679;
CHECKREG r3, 0xE58581B3;
CHECKREG r4, 0xEDE14569;
CHECKREG r5, 0xEDE10CB1;
CHECKREG r6, 0xFACEA70D;
CHECKREG r7, 0xFACDF209;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0x5354babd;
imm32 r1, 0x9dbcdec7;
imm32 r2, 0x7724e679;
imm32 r3, 0x80567007;
imm32 r4, 0x9fb34569;
imm32 r5, 0xa235200b;
imm32 r6, 0xb00c100d;
imm32 r7, 0x9876a10f;
 R0.H = A1 , A0 = R1.L * R0.L (IH);
R1 = A1.w;
 R2.H = A1 , A0 += R2.H * R3.L (IH);
R3 = A1.w;
 R4.H = A1 , A0 -= R4.H * R5.H (IH);
R5 = A1.w;
 R6.H = A1 , A0 += R6.L * R7.H (IH);
R7 = A1.w;
CHECKREG r0, 0xFACEBABD;
CHECKREG r1, 0xFACDF209;
CHECKREG r2, 0xFACEE679;
CHECKREG r3, 0xFACDF209;
CHECKREG r4, 0xFACE4569;
CHECKREG r5, 0xFACDF209;
CHECKREG r6, 0xFACE100D;
CHECKREG r7, 0xFACDF209;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x9dbcfec7;
imm32 r2, 0x81245679;
imm32 r3, 0x97060007;
imm32 r4, 0xaf6c4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00c400d;
imm32 r7, 0x678ed30f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 -= R1.L * R0.L (IH);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M), A0 += R2.H * R3.L (IH);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (M), A0 += R4.H * R5.H (IH);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M), A0 -= R6.L * R7.H (IH);
R7 = A1.w;
CHECKREG r0, 0xFF915ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x32945679;
CHECKREG r3, 0x329474C1;
CHECKREG r4, 0xD2A94569;
CHECKREG r5, 0xD2A949A4;
CHECKREG r6, 0xE621400D;
CHECKREG r7, 0xE6215AA8;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x92005ABD;
imm32 r1, 0x09300000;
imm32 r2, 0x56749679;
imm32 r3, 0x30A95000;
imm32 r4, 0xa0009669;
imm32 r5, 0x01000970;
imm32 r6, 0xdf45609D;
imm32 r7, 0x12345679;
R0.H = ( A1 -= R1.L * R0.L ) (M,IH);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M,IH);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (M,IH);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ) (M,IH);
R7 = A1.w;
CHECKREG r0, 0xE6215ABD;
CHECKREG r1, 0xE6215AA8;
CHECKREG r2, 0xD2129679;
CHECKREG r3, 0xD2126089;
CHECKREG r4, 0xFC769669;
CHECKREG r5, 0xFC760000;
CHECKREG r6, 0xFA22609D;
CHECKREG r7, 0xFA223404;



pass
