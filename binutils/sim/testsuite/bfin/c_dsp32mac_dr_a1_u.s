//Original:/testcases/core/c_dsp32mac_dr_a1_u/c_dsp32mac_dr_a1_u.dsp
// Spec Reference: dsp32mac dr_a1 u (unsigned fraction & unsigned int)
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
R0.H = ( A1 = R6.L * R7.L ), A0 += R6.L * R7.L (FU);
R1 = A1.w;
R2.H = ( A1 = R3.L * R4.H ), A0 = R3.H * R4.L (FU);
R3 = A1.w;
R4.H = ( A1 += R2.H * R5.L ), A0 = R2.H * R5.H (FU);
R5 = A1.w;
R6.H = ( A1 += R0.H * R1.H ), A0 += R0.L * R1.H (FU);
R7 = A1.w;
CHECKREG r0, 0x48665ABD;
CHECKREG r1, 0x486656C2;
CHECKREG r2, 0x86E08679;
CHECKREG r3, 0x86E04E24;
CHECKREG r4, 0xB651A569;
CHECKREG r5, 0xB650D9C4;
CHECKREG r6, 0xCACA80AD;
CHECKREG r7, 0xCACA6268;

imm32 r0, 0x03545abd;
imm32 r1, 0x1abcfec7;
imm32 r2, 0xc2a48679;
imm32 r3, 0x300a9007;
imm32 r4, 0x54bca569;
imm32 r5, 0x6d355a0b;
imm32 r6, 0x700c80ad;
imm32 r7, 0x878e900a;
R0.H = ( A1 -= R6.L * R7.L ), A0 += R6.L * R7.L (FU);
R1 = A1.w;
R2.H = ( A1 -= R3.L * R4.H ), A0 = R3.H * R4.L (FU);
R3 = A1.w;
R4.H = ( A1 += R2.H * R5.L ), A0 -= R2.H * R5.H (FU);
R5 = A1.w;
R6.H = ( A1 -= R0.H * R1.H ), A0 += R0.L * R1.H (FU);
R7 = A1.w;
CHECKREG r0, 0x82645ABD;
CHECKREG r1, 0x82640BA6;
CHECKREG r2, 0x52B88679;
CHECKREG r3, 0x52B7FA82;
CHECKREG r4, 0x6FD0A569;
CHECKREG r5, 0x6FD0386A;
CHECKREG r6, 0x2D6780AD;
CHECKREG r7, 0x2D66815A;

// The result accumulated in A1, and stored to a reg half (MNOP)
imm32 r0, 0xb3548abd;
imm32 r1, 0x7bbcfec7;
imm32 r2, 0xa1b45679;
imm32 r3, 0xb00b9007;
imm32 r4, 0xcfbcb569;
imm32 r5, 0xd235c00b;
imm32 r6, 0xe00cabbd;
imm32 r7, 0x678e700b;
R0.H = ( A1 = R1.L * R0.L ) (FU);
R1 = A1.w;
R2.H = ( A1 = R2.L * R6.H ) (FU);
R3 = A1.w;
R4.H = ( A1 += R3.H * R5.L ) (FU);
R5 = A1.w;
R6.H = ( A1 = R4.H * R7.H ) (FU);
R7 = A1.w;
CHECKREG r0, 0x8A138ABD;
CHECKREG r1, 0x8A135EEB;
CHECKREG r2, 0x4BAE5679;
CHECKREG r3, 0x4BADEDAC;
CHECKREG r4, 0x8473B569;
CHECKREG r5, 0x8472EE1B;
CHECKREG r6, 0x3594ABBD;
CHECKREG r7, 0x3593BCCA;

// The result accumulated in A1 , and stored to a reg half (MNOP)
imm32 r0, 0xc354babd;
imm32 r1, 0x6cbcdec7;
imm32 r2, 0x71c4e679;
imm32 r3, 0x800c7007;
imm32 r4, 0x9fbcc569;
imm32 r5, 0xa2359c0b;
imm32 r6, 0xb00c30cd;
imm32 r7, 0xc78ea00c;
 R0.H = A1 , A0 = R1.L * R0.L (FU);
R1 = A1.w;
 R2.H = A1 , A0 = R2.H * R3.L (FU);
R3 = A1.w;
 R4.H = A1 , A0 = R4.H * R5.H (FU);
R5 = A1.w;
 R6.H = A1 , A0 = R6.L * R7.H (FU);
R7 = A1.w;
CHECKREG r0, 0x3594BABD;
CHECKREG r1, 0x3593BCCA;
CHECKREG r2, 0x3594E679;
CHECKREG r3, 0x3593BCCA;
CHECKREG r4, 0x3594C569;
CHECKREG r5, 0x3593BCCA;
CHECKREG r6, 0x359430CD;
CHECKREG r7, 0x3593BCCA;

// The result accumulated in A1 , and stored to a reg half
imm32 r0, 0xd3545abd;
imm32 r1, 0x5dbcfec7;
imm32 r2, 0x71d45679;
imm32 r3, 0x900d0007;
imm32 r4, 0xafbcd569;
imm32 r5, 0xd2359d0b;
imm32 r6, 0xc00ca0dd;
imm32 r7, 0x678ed00d;
R0.H = ( A1 = R1.L * R2.L ) (M), A0 += R1.L * R2.L (FU);
R1 = A1.w;
R2.H = ( A1 = R3.L * R4.H ) (M), A0 = R3.H * R4.L (FU);
R3 = A1.w;
R4.H = ( A1 = R5.H * R6.L ) (M), A0 += R5.H * R6.H (FU);
R5 = A1.w;
R6.H = ( A1 += R7.H * R0.H ) (M), A0 += R7.L * R0.H (FU);
R7 = A1.w;
CHECKREG r0, 0xFF965ABD;
CHECKREG r1, 0xFF96460F;
CHECKREG r2, 0x00055679;
CHECKREG r3, 0x0004CE24;
CHECKREG r4, 0xE33AD569;
CHECKREG r5, 0xE33997C1;
CHECKREG r6, 0x4A9DA0DD;
CHECKREG r7, 0x4A9CB6F5;

// The result accumulated in A1 MM=0, and stored to a reg half (MNOP)
imm32 r0, 0xe3545abd;
imm32 r1, 0xaebcfec7;
imm32 r2, 0xc1e45679;
imm32 r3, 0x1c0e0007;
imm32 r4, 0xe1cce569;
imm32 r5, 0x921c0e0b;
imm32 r6, 0x790190ed;
imm32 r7, 0x679e900e;
R0.H = ( A1 = R1.L * R0.L ) (M,FU);
R1 = A1.w;
R2.H = ( A1 += R2.L * R3.H ) (M,FU);
R3 = A1.w;
R4.H = ( A1 += R4.H * R5.L ) (M,FU);
R5 = A1.w;
R6.H = ( A1 = R6.H * R7.H ) (M,FU);
R7 = A1.w;
CHECKREG r0, 0xFF915ABD;
CHECKREG r1, 0xFF910EEB;
CHECKREG r2, 0x090B5679;
CHECKREG r3, 0x090B0589;
CHECKREG r4, 0x0763E569;
CHECKREG r5, 0x0762E14D;
CHECKREG r6, 0x30FA90ED;
CHECKREG r7, 0x30FA159E;



pass
