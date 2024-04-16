//Original:/testcases/core/c_dsp32mac_dr_a1_is/c_dsp32mac_dr_a1_is.dsp
// Spec Reference: dsp32mac dr_a1 is ((scale by 2 signed int)
# mach: bfin

.include "testutils.inc"
	start




A1 = A0 = 0;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0xa3545abd;
imm32 r1, 0xbdbcfec7;
imm32 r2, 0xc1248679;
imm32 r3, 0xd0069007;
imm32 r4, 0xefbc4569;
imm32 r5, 0xcd35500b;
imm32 r6, 0xe00c800d;
imm32 r7, 0xf78e900f;
R0.H = ( A1 = R1.L * R0.L ), A0 = R1.L * R0.L (ISS2);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ), A0 = R2.H * R3.L (ISS2);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ), A0 -= R4.H * R5.H (ISS2);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ), A0 += R6.L * R7.H (ISS2);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x80008679;
CHECKREG r3, 0xE8CA9815;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0xE3B4A529;
CHECKREG r6, 0x8000800D;
CHECKREG r7, 0xE4C27FD1;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0x63548abd;
imm32 r1, 0x7dbcfec7;
imm32 r2, 0xC5885679;
imm32 r3, 0xC5880000;
imm32 r4, 0xcfbc4569;
imm32 r5, 0xd235c00b;
imm32 r6, 0xe00ca00d;
imm32 r7, 0x678e700f;
R0.H = ( A1 = R1.L * R0.L ) (ISS2);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (ISS2);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (ISS2);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (ISS2);
R7 = A1.w;
CHECKREG r0, 0x7FFF8ABD;
CHECKREG r1, 0x008F5EEB;
CHECKREG r2, 0x80005679;
CHECKREG r3, 0xECCF6C33;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0xE0C07F1F;
CHECKREG r6, 0x8000A00D;
CHECKREG r7, 0xEDAD6477;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0x5354babd;
imm32 r1, 0x6dbcdec7;
imm32 r2, 0x7124e679;
imm32 r3, 0x80067007;
imm32 r4, 0x9fbc4569;
imm32 r5, 0xa235900b;
imm32 r6, 0xb00c300d;
imm32 r7, 0xc78ea00f;
 R0.H = A1 , A0 -= R1.L * R0.L (ISS2);
R1 = A1.w;
 R2.H = A1 , A0 += R2.H * R3.L (ISS2);
R3 = A1.w;
 R4.H = A1 , A0 -= R4.H * R5.H (ISS2);
R5 = A1.w;
 R6.H = A1 , A0 = R6.L * R7.H (ISS2);
R7 = A1.w;
CHECKREG r0, 0x8000BABD;
CHECKREG r1, 0xEDAD6477;
CHECKREG r2, 0x8000E679;
CHECKREG r3, 0xEDAD6477;
CHECKREG r4, 0x80004569;
CHECKREG r5, 0xEDAD6477;
CHECKREG r6, 0x8000300D;
CHECKREG r7, 0xEDAD6477;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 = R1.L * R0.L (ISS2);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M), A0 -= R2.H * R3.L (ISS2);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (M), A0 += R4.H * R5.H (ISS2);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ) (M), A0 += R6.L * R7.H (ISS2);
R7 = A1.w;
CHECKREG r0, 0x80005ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x7FFF5679;
CHECKREG r3, 0x303725C1;
CHECKREG r4, 0x7FFF4569;
CHECKREG r5, 0x5D60D8AD;
CHECKREG r6, 0x7FFFA00D;
CHECKREG r7, 0x43823355;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0x92005ABD;
imm32 r1, 0x09300000;
imm32 r2, 0x56749679;
imm32 r3, 0x30A95000;
imm32 r4, 0xa0009669;
imm32 r5, 0x01000970;
imm32 r6, 0xdf45609D;
imm32 r7, 0x12345679;
R0.H = ( A1 += R1.L * R0.L ) (M,ISS2);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M,ISS2);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (M,ISS2);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M,ISS2);
R7 = A1.w;
CHECKREG r0, 0x7FFF5ABD;
CHECKREG r1, 0x43823355;
CHECKREG r2, 0x7FFF9679;
CHECKREG r3, 0x57912D74;
CHECKREG r4, 0x7FFF9669;
CHECKREG r5, 0x5B1B2D74;
CHECKREG r6, 0x8000609D;
CHECKREG r7, 0xFDAC3404;



pass
