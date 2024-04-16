//Original:/testcases/core/c_dsp32mult_dr_mix/c_dsp32mult_dr_mix.dsp
// Spec Reference: dsp32mult single dr (mix) u i t is tu ih
# mach: bfin

.include "testutils.inc"
	start

// test the default (signed fraction) rounding U=0 I=0 T=0
imm32 r0, 0xab235615;
imm32 r1, 0xcfba5117;
imm32 r2, 0x13246715;
imm32 r3, 0x00060017;
imm32 r4, 0x90abcd19;
imm32 r5, 0x10acef1b;
imm32 r6, 0x000c001d;
imm32 r7, 0x1246701f;
R2.H = R1.L * R0.L, R2.L = R1.L * R0.L;
R3.L = R1.L * R0.H (ISS2);
R4.H = R1.H * R0.L;
R5.H = R1.L * R0.H (M), R5.L = R1.H * R0.H;
R6.H = R1.H * R0.L, R6.L = R1.L * R0.L;
R7.H = R1.H * R0.H (M), R7.L = R1.H * R0.H;
CHECKREG r2, 0x36893689;
CHECKREG r3, 0x00068000;
CHECKREG r4, 0xDF89CD19;
CHECKREG r5, 0x36352001;
CHECKREG r6, 0xDF893689;
CHECKREG r7, 0xDFBB2001;

// test the signed integer U=0 I=1
imm32 r0, 0x8b235625;
imm32 r1, 0x9fba5127;
imm32 r2, 0xa3246725;
imm32 r3, 0x00060027;
imm32 r4, 0xb0abcd29;
imm32 r5, 0x10acef2b;
imm32 r6, 0xc00c002d;
imm32 r7, 0xd246702f;
R2.H = R1.L * R0.L, R2.L = R1.L * R0.L (TFU);
R3.H = R1.L * R0.L, R3.L = R1.L * R0.H (IS);
R4.H = R1.L * R0.L, R4.L = R1.H * R0.L (ISS2);
R5.H = R1.L * R0.L, R5.L = R1.H * R0.H (IS);
R6.H = R1.L * R0.H, R6.L = R1.L * R0.L (IS);
R7.H = R1.L * R0.H, R7.L = R1.L * R0.H (IH);
CHECKREG r0, 0x8B235625;
CHECKREG r1, 0x9FBA5127;
CHECKREG r2, 0x1B4E1B4E;
CHECKREG r3, 0x7FFF8000;
CHECKREG r4, 0x7FFF8000;
CHECKREG r5, 0x7FFF7FFF;
CHECKREG r6, 0x80007FFF;
CHECKREG r7, 0xDAF4DAF4;

imm32 r0, 0x5b23a635;
imm32 r1, 0x6fba5137;
imm32 r2, 0x1324b735;
imm32 r3, 0x90060037;
imm32 r4, 0x80abcd39;
imm32 r5, 0xb0acef3b;
imm32 r6, 0xa00c003d;
imm32 r7, 0x12467003;
R0.H = R3.L * R2.H, R0.L = R3.H * R2.L (IS);
R1.H = R3.L * R2.H, R1.L = R3.H * R2.H (ISS2);
R4.H = R3.H * R2.L, R4.L = R3.L * R2.L (IS);
R5.H = R3.H * R2.L, R5.L = R3.L * R2.H (IS);
R6.H = R3.H * R2.L, R6.L = R3.H * R2.L (IH);
R7.H = R3.H * R2.L, R7.L = R3.H * R2.H (IS);
CHECKREG r0, 0x7FFF7FFF;
CHECKREG r1, 0x7FFF8000;
CHECKREG r2, 0x1324B735;
CHECKREG r3, 0x90060037;
CHECKREG r4, 0x7FFF8000;
CHECKREG r5, 0x7FFF7FFF;
CHECKREG r6, 0x1FD71FD7;
CHECKREG r7, 0x7FFF8000;

imm32 r0, 0x1b235655;
imm32 r1, 0xc4ba5157;
imm32 r2, 0x63246755;
imm32 r3, 0x00060055;
imm32 r4, 0x90abc509;
imm32 r5, 0x10acef5b;
imm32 r6, 0xb00c005d;
imm32 r7, 0x1246705f;
R0.H = R5.H * R4.H, R0.L = R5.L * R4.L (IS);
R1.H = R5.H * R4.H, R1.L = R5.L * R4.H (ISS2);
R2.H = R5.H * R4.H, R2.L = R5.H * R4.L (IS);
R3.H = R5.H * R4.H, R3.L = R5.H * R4.H (IS);
R4.H = R6.H * R7.L, R4.L = R6.H * R7.L (IH);
R5.H = R6.L * R7.H, R5.L = R6.H * R7.H (IS);
CHECKREG r0, 0x80007FFF;
CHECKREG r1, 0x80007FFF;
CHECKREG r2, 0x80008000;
CHECKREG r3, 0x80008000;
CHECKREG r4, 0xDCE8DCE8;
CHECKREG r5, 0x7FFF8000;
CHECKREG r6, 0xB00C005D;
CHECKREG r7, 0x1246705F;

imm32 r0, 0xbb235666;
imm32 r1, 0xefba5166;
imm32 r2, 0x13248766;
imm32 r3, 0xf0060066;
imm32 r4, 0x90ab9d69;
imm32 r5, 0x10acef6b;
imm32 r6, 0x800cb06d;
imm32 r7, 0x1246706f;
// test the unsigned U=1
R2.H = R1.L * R0.L, R2.L = R1.L * R0.L (FU);
R3.H = R1.L * R0.L, R3.L = R1.L * R0.H (ISS2);
R4.H = R7.L * R6.L, R4.L = R7.H * R6.L (FU);
R5.H = R3.L * R2.L (M), R5.L = R3.H * R2.H (FU);
R6.H = R5.L * R4.H, R6.L = R5.L * R4.L (TFU);
R7.H = R5.L * R4.H, R7.L = R5.L * R4.H (FU);
CHECKREG r0, 0xBB235666;
CHECKREG r1, 0xEFBA5166;
CHECKREG r2, 0x1B791B79;
CHECKREG r3, 0x7FFF8000;
CHECKREG r4, 0x4D7C0C98;
CHECKREG r5, 0xF2440DBC;
CHECKREG r6, 0x042800AC;
CHECKREG r7, 0x04280428;

imm32 r0, 0xab23a675;
imm32 r1, 0xcfba5127;
imm32 r2, 0x13246705;
imm32 r3, 0x00060007;
imm32 r4, 0x90abcd09;
imm32 r5, 0x10acdfdb;
imm32 r6, 0x000c000d;
imm32 r7, 0x1246f00f;
R0.H = R5.L * R4.H, R0.L = R5.H * R4.L (FU);
R1.H = R3.L * R2.H, R1.L = R3.H * R2.H (IU);
R2.H = R7.H * R6.L, R2.L = R7.L * R6.L (TFU);
R3.H = R5.H * R4.L, R3.L = R5.L * R4.H (FU);
R6.H = R1.H * R0.L, R6.L = R1.H * R0.L (IH);
R7.H = R3.H * R2.L, R7.L = R3.H * R2.H (FU);
CHECKREG r0, 0x7E810D5A;
CHECKREG r1, 0x85FC72D8;
CHECKREG r2, 0x0000000C;
CHECKREG r3, 0x0D5A7E81;
CHECKREG r4, 0x90ABCD09;
CHECKREG r5, 0x10ACDFDB;
CHECKREG r6, 0xF9A3F9A3;
CHECKREG r7, 0x00010000;

imm32 r0, 0xab235a75;
imm32 r1, 0xcfba5127;
imm32 r2, 0x13246905;
imm32 r3, 0x00060007;
imm32 r4, 0x90abcd09;
imm32 r5, 0x10ace9db;
imm32 r6, 0x000c0d0d;
imm32 r7, 0x1246700f;
R2.H = R1.H * R0.H, R2.L = R1.L * R0.L (TFU);
R3.H = R1.H * R0.L, R3.L = R1.L * R0.H (FU);
R4.H = R6.H * R7.H, R4.L = R6.H * R7.L (ISS2);
R5.H = R6.L * R7.H, R5.L = R6.H * R7.H (FU);
CHECKREG r0, 0xAB235A75;
CHECKREG r1, 0xCFBA5127;
CHECKREG r2, 0x8ADD1CAC;
CHECKREG r3, 0x49663640;
CHECKREG r4, 0x7FFF7FFF;
CHECKREG r5, 0x00EE0001;
CHECKREG r6, 0x000C0D0D;
CHECKREG r7, 0x1246700F;

// test the ROUNDING only on signed fraction T=1
imm32 r0, 0xab235675;
imm32 r1, 0xcfba5127;
imm32 r2, 0x13246705;
imm32 r3, 0x00060007;
imm32 r4, 0x90abcd09;
imm32 r5, 0x10acefdb;
imm32 r6, 0x000c000d;
imm32 r7, 0x1246700f;
R2.H = R1.L * R0.L (M), R2.L = R1.L * R0.H (IS);
R3.H = R1.H * R0.L (M), R3.L = R1.H * R0.H (FU);
R0.H = R3.L * R2.L (M), R0.L = R3.H * R2.H (T);
R1.H = R5.L * R4.H (M), R1.L = R5.L * R4.L (S2RND);
R4.H = R7.H * R6.H (M), R4.L = R7.L * R6.L (IU);
R5.H = R7.L * R6.H (M), R5.L = R7.H * R6.L (TFU);
R6.H = R5.H * R4.L (M), R6.L = R5.L * R4.H (ISS2);
R7.H = R3.L * R2.H (M), R7.L = R3.L * R2.L (IH);
CHECKREG r0, 0xC56FEFB2;
CHECKREG r1, 0xEDC10CDB;
CHECKREG r2, 0x7FFF8000;
CHECKREG r3, 0xEFB28ADE;
CHECKREG r4, 0x7FFFFFFF;
CHECKREG r5, 0x00050000;
CHECKREG r6, 0x7FFF0000;
CHECKREG r7, 0xC56F3A91;



pass
