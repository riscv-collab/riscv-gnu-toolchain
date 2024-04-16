//Original:/testcases/core/c_dsp32mac_dr_a1_tu/c_dsp32mac_dr_a1_tu.dsp
// Spec Reference: dsp32mac dr_a1 tu (truncate signed fraction)
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
R0.H = ( A1 = R1.L * R0.L ), A0 = R1.L * R0.L (TFU);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ), A0 = R2.H * R3.L (TFU);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ), A0 -= R4.H * R5.H (TFU);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ), A0 += R6.L * R7.H (TFU);
R7 = A1.w;
CHECKREG r0, 0x5A4E5ABD;
CHECKREG r1, 0x5A4E0EEB;
CHECKREG r2, 0x00008679;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x4AF54569;
CHECKREG r5, 0x4AF50D14;
CHECKREG r6, 0xFFFF800D;
CHECKREG r7, 0x239CE7BC;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0x63548abd;
imm32 r1, 0x7dbcfec7;
imm32 r2, 0xC5885679;
imm32 r3, 0xC5880000;
imm32 r4, 0xcfbc4569;
imm32 r5, 0xd235c00b;
imm32 r6, 0xe00ca00d;
imm32 r7, 0x678e700f;
R0.H = ( A1 = R1.L * R0.L ) (TFU);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (TFU);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (TFU);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (TFU);
R7 = A1.w;
CHECKREG r0, 0x8A138ABD;
CHECKREG r1, 0x8A135EEB;
CHECKREG r2, 0xCCCC5679;
CHECKREG r3, 0xCCCC6C33;
CHECKREG r4, 0x30F64569;
CHECKREG r5, 0x30F67F1F;
CHECKREG r6, 0x5AA1A00D;
CHECKREG r7, 0x5AA11AA8;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0x5354babd;
imm32 r1, 0x6dbcdec7;
imm32 r2, 0x7124e679;
imm32 r3, 0x80067007;
imm32 r4, 0x9fbc4569;
imm32 r5, 0xa235900b;
imm32 r6, 0xb00c300d;
imm32 r7, 0xc78ea00f;
 R0.H = A1 , A0 -= R1.L * R0.L (TFU);
R1 = A1.w;
 R2.H = A1 , A0 += R2.H * R3.L (TFU);
R3 = A1.w;
 R4.H = A1 , A0 -= R4.H * R5.H (TFU);
R5 = A1.w;
 R6.H = A1 , A0 = R6.L * R7.H (TFU);
R7 = A1.w;
CHECKREG r0, 0x5AA1BABD;
CHECKREG r1, 0x5AA11AA8;
CHECKREG r2, 0x5AA1E679;
CHECKREG r3, 0x5AA11AA8;
CHECKREG r4, 0x5AA14569;
CHECKREG r5, 0x5AA11AA8;
CHECKREG r6, 0x5AA1300D;
CHECKREG r7, 0x5AA11AA8;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
R0.H = ( A1 = R1.L * R0.L ) (M), A0 -= R1.L * R0.L (TFU);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M), A0 -= R2.H * R3.L (TFU);
R3 = A1.w;
R4.H = ( A1 -= R4.H * R5.L ) (M), A0 += R4.H * R5.H (TFU);
R5 = A1.w;
R6.H = ( A1 += R6.H * R7.H ) (M), A0 += R6.L * R7.H (TFU);
R7 = A1.w;
CHECKREG r0, 0xFF915ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x30375679;
CHECKREG r3, 0x303725C1;
CHECKREG r4, 0x5D604569;
CHECKREG r5, 0x5D60D8AD;
CHECKREG r6, 0x4382A00D;
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
R0.H = ( A1 += R1.L * R0.L ) (M,TFU);
R1 = A1.w;
R2.H = ( A1 -= R2.L * R3.H ) (M,TFU);
R3 = A1.w;
R4.H = ( A1 = R4.H * R5.L ) (M,TFU);
R5 = A1.w;
R6.H = ( A1 -= R6.H * R7.H ) (M,TFU);
R7 = A1.w;
CHECKREG r0, 0x43825ABD;
CHECKREG r1, 0x43823355;
CHECKREG r2, 0x57919679;
CHECKREG r3, 0x57912D74;
CHECKREG r4, 0xFC769669;
CHECKREG r5, 0xFC760000;
CHECKREG r6, 0xFEC9609D;
CHECKREG r7, 0xFEC9CBFC;



pass
