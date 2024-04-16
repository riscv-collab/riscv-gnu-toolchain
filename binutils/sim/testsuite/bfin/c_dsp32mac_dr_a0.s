//Original:/testcases/core/c_dsp32mac_dr_a0/c_dsp32mac_dr_a0.dsp
// Spec Reference: dsp32mac dr_a0
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0xab235675;
imm32 r1, 0xcaba5127;
imm32 r2, 0x13a46705;
imm32 r3, 0x000a0007;
imm32 r4, 0x90abad09;
imm32 r5, 0x10aceadb;
imm32 r6, 0x000c00ad;
imm32 r7, 0x1246700a;

A1 = A0 = 0;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0xb3545abd;
imm32 r1, 0xabbcfec7;
imm32 r2, 0xa1b45679;
imm32 r3, 0x000b0007;
imm32 r4, 0xefbcb569;
imm32 r5, 0x12350b0b;
imm32 r6, 0x000c00bd;
imm32 r7, 0x678e000b;
A1 = R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
A1 -= R2.L * R3.L, R2.L = ( A0 = R2.H * R3.L );
R3 = A0.w;
A1 = R4.L * R5.L, R4.L = ( A0 += R4.H * R5.H );
R5 = A0.w;
A1 = R6.L * R7.L, R6.L = ( A0 = R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0xB354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xA1B4FFFB;
CHECKREG r3, 0xFFFAD7D8;
CHECKREG r4, 0xEFBCFDAB;
CHECKREG r5, 0xFDAA8BB0;
CHECKREG r6, 0x000C0099;
CHECKREG r7, 0x0098E7AC;

imm32 r0, 0xc3545abd;
imm32 r1, 0xacbcfec7;
imm32 r2, 0xa1c45679;
imm32 r3, 0x000c0007;
imm32 r4, 0xefbcc569;
imm32 r5, 0x12350c0b;
imm32 r6, 0x000c00cd;
imm32 r7, 0x678e000c;
A1 = R1.L * R0.H, R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
A1 -= R2.L * R3.H, R2.L = ( A0 -= R2.H * R3.L );
R3 = A0.w;
A1 = R4.H * R5.H, R4.L = ( A0 += R4.H * R5.H );
R5 = A0.w;
A1 -= R6.H * R7.H, R6.L = ( A0 += R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0xC354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xA1C4FF27;
CHECKREG r3, 0xFF27451E;
CHECKREG r4, 0xEFBCFCD7;
CHECKREG r5, 0xFCD6F8F6;
CHECKREG r6, 0x000CFD7D;
CHECKREG r7, 0xFD7CD262;

imm32 r0, 0xd3545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1d45679;
imm32 r3, 0x000d0007;
imm32 r4, 0xefbcd569;
imm32 r5, 0x12350d0b;
imm32 r6, 0x000c00dd;
imm32 r7, 0x678e000d;
A1 += R1.H * R0.L, R0.L = ( A0 -= R1.L * R0.L );
R1 = A0.w;
A1 = R2.H * R3.H, R2.L = ( A0 -= R2.H * R3.L );
R3 = A0.w;
A1 -= R4.H * R5.L, R4.L = ( A0 -= R4.H * R5.H );
R5 = A0.w;
A1 += R6.H * R7.L, R6.L = ( A0 = R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0xD354FE5B;
CHECKREG r1, 0xFE5AB48C;
CHECKREG r2, 0xA1D4FE60;
CHECKREG r3, 0xFE5FDAF4;
CHECKREG r4, 0xEFBC00B0;
CHECKREG r5, 0x00B0271C;
CHECKREG r6, 0x000C00B3;
CHECKREG r7, 0x00B2CB2C;

imm32 r0, 0xe3545abd;
imm32 r1, 0xaebcfec7;
imm32 r2, 0xa1e45679;
imm32 r3, 0x000e0007;
imm32 r4, 0xefbce569;
imm32 r5, 0x12350e0b;
imm32 r6, 0x000c00ed;
imm32 r7, 0x678e000e;
A1 = R1.H * R0.H, R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
A1 += R2.H * R3.H, R2.L = ( A0 += R2.H * R3.L );
R3 = A0.w;
A1 = R4.H * R5.H, R4.L = ( A0 = R4.H * R5.H );
R5 = A0.w;
A1 = R6.H * R7.H, R6.L = ( A0 -= R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0xE354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xA1E4FF1D;
CHECKREG r3, 0xFF1CF84E;
CHECKREG r4, 0xEFBCFDB0;
CHECKREG r5, 0xFDAFB3D8;
CHECKREG r6, 0x000CFCF0;
CHECKREG r7, 0xFCEFF6EC;


pass
