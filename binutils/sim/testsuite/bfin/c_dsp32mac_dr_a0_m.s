//Original:/testcases/core/c_dsp32mac_dr_a0_m/c_dsp32mac_dr_a0_m.dsp
// Spec Reference: dsp32mac dr_a0 m
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
A1 -= R1.L * R0.L, R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
A1 = R2.L * R3.H, R2.L = ( A0 -= R2.H * R3.L );
R3 = A0.w;
A1 = R4.H * R5.L, R4.L = ( A0 += R4.H * R5.H );
R5 = A0.w;
A1 = R6.H * R7.H, R6.L = ( A0 = R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0x1354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xA124FF27;
CHECKREG r3, 0xFF274DDE;
CHECKREG r4, 0xEFBCFCD7;
CHECKREG r5, 0xFCD701B6;
CHECKREG r6, 0x000C000B;
CHECKREG r7, 0x000A846C;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
R0.L = ( A0 += R6.L * R7.L );
R1 = A0.w;
R2.L = ( A0 -= R2.L * R3.H );
R3 = A0.w;
R4.L = ( A0 += R4.H * R5.L );
R5 = A0.w;
R6.L = ( A0 = R0.H * R1.H );
R7 = A0.w;
CHECKREG r0, 0x1354000B;
CHECKREG r1, 0x000A85F2;
CHECKREG r2, 0xA1240006;
CHECKREG r3, 0x00067846;
CHECKREG r4, 0xEFBC0005;
CHECKREG r5, 0x0005126E;
CHECKREG r6, 0x000C0002;
CHECKREG r7, 0x00018290;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
R0.L = ( A0 = R1.L * R0.L );
R1 = A0.w;
R2.L = ( A0 += R2.H * R3.L );
R3 = A0.w;
R4.L = ( A0 += R4.H * R5.H );
R5 = A0.w;
R6.L = ( A0 += R6.L * R7.H );
R7 = A0.w;
CHECKREG r0, 0x1354FF22;
CHECKREG r1, 0xFF221DD6;
CHECKREG r2, 0xA124FF1D;
CHECKREG r3, 0xFF1CEDCE;
CHECKREG r4, 0xEFBCFCCD;
CHECKREG r5, 0xFCCCA1A6;
CHECKREG r6, 0x000CFCD7;
CHECKREG r7, 0xFCD72612;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
A1 = R1.L * R0.L (M), R6.L = ( A0 -= R1.L * R0.L );
R7 = A0.w;
A1 -= R2.L * R3.H (M), R2.L = ( A0 += R2.H * R3.L );
R3 = A0.w;
A1 = R4.H * R5.L (M), R4.L = ( A0 = R4.H * R5.H );
R5 = A0.w;
A1 -= R6.H * R7.H (M), R0.L = ( A0 = R6.L * R7.H );
R1 = A0.w;
CHECKREG r0, 0x1354000B;
CHECKREG r1, 0x000A83F2;
CHECKREG r2, 0xA124FDB0;
CHECKREG r3, 0xFDAFD834;
CHECKREG r4, 0xEFBCFDB0;
CHECKREG r5, 0xFDAFB3D8;
CHECKREG r6, 0x000CFDB5;
CHECKREG r7, 0xFDB5083C;


pass
