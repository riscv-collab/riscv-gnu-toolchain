//Original:/testcases/core/c_dsp32mac_mix/c_dsp32mac_mix.dsp
// Spec Reference: dsp32mac mix
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
A0.L = R0.L;
A0.H = R0.H;

// test the ROUNDING only on signed fraction T=1
R0.H = (A1 = R4.L * R5.L), R0.L = (A0 = R4.L * R5.H) (T);
R1.H = (A1 = R4.H * R5.L), R1.L = (A0 = R4.H * R5.H) (T);
R2.H = (A1 = R6.L * R7.L), R2.L = (A0 = R6.H * R7.H) (T);
R3.H = (A1 = R6.L * R7.H), R3.L = (A0 = R6.L * R7.L) (T);
CHECKREG r0, 0x066DF95C;
CHECKREG r1, 0x0E0AF17F;
CHECKREG r2, 0x000B0001;
CHECKREG r3, 0x0001000B;

// When two results are stored to a single register, they must be rounded
// or truncated and stored to the 2 halves of a single destination reg dst

imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;

// The result accumulated in A0 and A1, and stored to a reg half
R2.H = ( A1 = R1.L * R0.H ), A0 = R1.H * R0.L;
R3.H = A1 , A0 = R7.H * R6.L (T);
R4.H = ( A1 = R3.L * R2.H ) (M), A0 = R3.H * R2.L;
A1 = R1.L * R0.H, R5.L = ( A0 = R1.H * R0.L ) (ISS2);

CHECKREG r2, 0xFFD15679;
CHECKREG r3, 0xFFD00007;
CHECKREG r4, 0x00074569;
CHECKREG r5, 0x12358000;

imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
// The result accumulated in A0 and A1, and stored to a reg
R5.H = (A1 = R1.L * R0.H), R5.L = (A0 = R1.H * R0.L) (TFU);
R6.H = (A1 = R3.L * R2.H) (M), R6.L = (A0 = R3.H * R2.L) (TFU);
R7.H = (A1 = R1.L * R0.H) (M), R7.L = (A0 = R1.H * R0.L) (IH);	// hi-word extraction
CHECKREG r5, 0x133C3D94;
CHECKREG r6, 0x00040002;
CHECKREG r7, 0xFFE8E2D7;


// The result accumulated in A0 and A1, and stored to a reg pair
imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;

R3 = ( A1 = R1.L * R0.H ), A0 = R1.H * R0.L;
R5 = ( A1 = R1.L * R0.H );
R7 = ( A1 += R1.L * R0.H ) (M), A0 -= R1.H * R0.L;
CHECKREG r2, 0xA1245679;
CHECKREG r3, 0xFFD0BC98;
CHECKREG r4, 0xEFBC4569;
CHECKREG r5, 0xFFD0BC98;
CHECKREG r6, 0x000C000D;
CHECKREG r7, 0xFFB91AE4;
A1 = R1.L * R0.H, R2 = ( A0 = R1.H * R0.L );
A1 = R1.L * R0.H (M), R6 = ( A0 -= R1.H * R0.L );
CHECKREG r2, 0xC5AEB798;
CHECKREG r3, 0xFFD0BC98;
CHECKREG r4, 0xEFBC4569;
CHECKREG r5, 0xFFD0BC98;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0xFFB91AE4;

imm32 r0, 0x13545abd;
imm32 r1, 0xadbcfec7;
imm32 r2, 0xa1245679;
imm32 r3, 0x00060007;
imm32 r4, 0xefbc4569;
imm32 r5, 0x1235000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x678e000f;
R3 = ( A1 -= R5.L * R4.H ), R2 = ( A0 -= R5.H * R4.L ) (S2RND);
R3 = ( A1 -= R1.L * R0.H ) (M), R2 = ( A0 += R1.H * R0.L ) (S2RND);
CHECKREG r2, 0x80000000;
CHECKREG r3, 0x0002CBB0;



pass
