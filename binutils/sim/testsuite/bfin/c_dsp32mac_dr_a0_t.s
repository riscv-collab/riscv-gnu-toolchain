//Original:/testcases/core/c_dsp32mac_dr_a0_t/c_dsp32mac_dr_a0_t.dsp
// Spec Reference: dsp32mac dr a0 t (truncation)
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
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L ) (T);
R1 = A0.w;
A1 -= R2.L * R3.H, R2.L = ( A0 = R2.H * R3.L ) (T);
R3 = A0.w;
A1 -= R4.H * R5.L, R4.L = ( A0 += R4.H * R5.H ) (T);
R5 = A0.w;
A1 = R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H ) (T);
R7 = A0.w;
CHECKREG r0, 0xA354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xC12436FD;
CHECKREG r3, 0x36FD0FF8;
CHECKREG r4, 0xEFBC3D71;
CHECKREG r5, 0x3D716BD0;
CHECKREG r6, 0xE00C45E2;
CHECKREG r7, 0x45E2903C;

// The result accumulated in A , and stored to a reg half (MNOP)
imm32 r0, 0x63548abd;
imm32 r1, 0x7dbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0xb0069007;
imm32 r4, 0xcfbc4569;
imm32 r5, 0xd235c00b;
imm32 r6, 0xe00ca00d;
imm32 r7, 0x678e700f;
R0.L = ( A0 = R1.L * R0.L ) (T);
R1 = A0.w;
R2.L = ( A0 += R2.L * R3.H ) (T);
R3 = A0.w;
R4.L = ( A0 -= R4.H * R5.L ) (T);
R5 = A0.w;
R6.L = ( A0 = R6.H * R7.H ) (T);
R7 = A0.w;
CHECKREG r0, 0x6354011E;
CHECKREG r1, 0x011EBDD6;
CHECKREG r2, 0xA124CB17;
CHECKREG r3, 0xCB172B82;
CHECKREG r4, 0xCFBCB2F9;
CHECKREG r5, 0xB2F9515A;
CHECKREG r6, 0xE00CE626;
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
R0.L = ( A0 -= R1.L * R0.L ) (T);
R1 = A0.w;
R2.L = ( A0 = R2.H * R3.L ) (T);
R3 = A0.w;
R4.L = ( A0 -= R4.H * R5.H ) (T);
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H ) (T);
R7 = A0.w;
CHECKREG r0, 0x5354D42C;
CHECKREG r1, 0xD42C177A;
CHECKREG r2, 0x71246305;
CHECKREG r3, 0x6305AFF8;
CHECKREG r4, 0x9FBC1C7B;
CHECKREG r5, 0x1C7B9C20;
CHECKREG r6, 0xB00C074B;
CHECKREG r7, 0x074B208C;

// The result accumulated in A , and stored to a reg half
imm32 r0, 0x33545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71245679;
imm32 r3, 0x90060007;
imm32 r4, 0xafbc4569;
imm32 r5, 0xd235900b;
imm32 r6, 0xc00ca00d;
imm32 r7, 0x678ed00f;
A1 = R1.L * R0.L (M), R0.L = ( A0 += R1.L * R0.L ) (T);
R1 = A0.w;
A1 += R2.L * R3.H (M), R2.L = ( A0 -= R2.H * R3.L ) (T);
R3 = A0.w;
A1 += R4.H * R5.L (M), R4.L = ( A0 = R4.H * R5.H ) (T);
R5 = A0.w;
A1 -= R6.H * R7.H (M), R6.L = ( A0 += R6.L * R7.H ) (T);
R7 = A0.w;
CHECKREG r0, 0x3354066D;
CHECKREG r1, 0x066D3E62;
CHECKREG r2, 0x71240667;
CHECKREG r3, 0x06670E6A;
CHECKREG r4, 0xAFBC1CB7;
CHECKREG r5, 0x1CB733D8;
CHECKREG r6, 0xC00CCF17;
CHECKREG r7, 0xCF173844;



pass
