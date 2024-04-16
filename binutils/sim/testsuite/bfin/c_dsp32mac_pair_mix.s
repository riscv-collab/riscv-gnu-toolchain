//Original:/testcases/core/c_dsp32mac_pair_mix/c_dsp32mac_pair_mix.dsp
// Spec Reference: dsp32mac pair mix
# mach: bfin

.include "testutils.inc"
	start



imm32 r0, 0x00000000;
imm32 r1, 0x00060007;
imm32 r2, 0x00040005;
imm32 r3, 0x00060007;
imm32 r4, 0x00080009;
imm32 r5, 0x000a000b;
imm32 r6, 0x000c000d;
imm32 r7, 0x000e000f;

A0 = 0;
ASTAT = R0;
// The result accumulated in A0 and A1, and stored to a reg pair
imm32 r0, 0x00120034;
imm32 r1, 0x00050006;

R3 = ( A1 = R1.L * R0.H ), A0 = R1.H * R0.L;
R5 = ( A1 = R1.L * R0.H );
R7 = ( A1 = R1.L * R0.H ) (M), A0 = R1.H * R0.L;
CHECKREG r2, 0x00040005;
CHECKREG r3, 0x000000d8;
CHECKREG r4, 0x00080009;
CHECKREG r5, 0x000000d8;
CHECKREG r6, 0x000C000D;
CHECKREG r7, 0x0000006c;
A1 = R1.L * R0.H, R2 = ( A0 += R1.H * R0.L );
A1 = R1.L * R0.H (M), R6 = ( A0 -= R1.H * R0.L );
CHECKREG r2, 0x00000410;
CHECKREG r3, 0x000000d8;
CHECKREG r4, 0x00080009;
CHECKREG r5, 0x000000d8;
CHECKREG r6, 0x00000208;
CHECKREG r7, 0x0000006c;
R3 = ( A1 = R1.L * R0.H ), R2 = ( A0 += R1.H * R0.L ) (S2RND);
R5 = ( A1 = R1.L * R0.H ) (M), R4 = ( A0 -= R1.H * R0.L ) (S2RND);
CHECKREG r2, 0x00000820;
CHECKREG r3, 0x000001B0;
CHECKREG r4, 0x00000410;
CHECKREG r5, 0x000000D8;

imm32 r0, 0x12345678;
imm32 r1, 0x34567897;
imm32 r2, 0x0acb1234;
imm32 r3, 0x456acb07;
imm32 r4, 0x421dbc09;
imm32 r5, 0x89acbd0b;
imm32 r6, 0x5adbcd0d;
imm32 r7, 0x9abc230f;
A1 += R7.L * R5.H, R2 = ( A0 = R7.H * R5.L );
A1 -= R1.H * R2.L (M), R6 = ( A0 += R1.L * R2.H ) (S2RND);
CHECKREG r0, 0x12345678;
CHECKREG r1, 0x34567897;
CHECKREG r2, 0x34F8E428;
CHECKREG r3, 0x456ACB07;
CHECKREG r4, 0x421DBC09;
CHECKREG r5, 0x89ACBD0B;
CHECKREG r6, 0x7FFFFFFF;
CHECKREG r7, 0x9ABC230F;


pass
