//Original:/testcases/core/c_dsp32alu_r_lh_a0pa1/c_dsp32alu_r_lh_a0pa1.dsp
// Spec Reference: dsp32alu r(lh) = ( a0 += a1)
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x15678911;
imm32 r1, 0x0125ab2d;
imm32 r2, 0x04445535;
imm32 r3, 0x00567747;
imm32 r4, 0x0566895b;
imm32 r5, 0x07897b6d;
imm32 r6, 0x04445875;
imm32 r7, 0x06667797;
A0 = R0;
A1 = R1;
R0 = ( A0 += A1 );
R1 = ( A0 += A1 );
R2 = ( A0 += A1 );
R3 = ( A0 += A1 );
R4 = ( A0 += A1 );
R5 = ( A0 += A1 );
R6 = ( A0 += A1 );
R7 = ( A0 += A1 );
CHECKREG r0, 0x168D343E;
CHECKREG r1, 0x17B2DF6B;
CHECKREG r2, 0x18D88A98;
CHECKREG r3, 0x19FE35C5;
CHECKREG r4, 0x1B23E0F2;
CHECKREG r5, 0x1C498C1F;
CHECKREG r6, 0x1D6F374C;
CHECKREG r7, 0x1E94E279;

imm32 r0, 0x068D343E;
imm32 r1, 0x02B2DF6B;
imm32 r2, 0x48388A98;
imm32 r3, 0x59F435C5;
imm32 r4, 0x6B25E0F2;
imm32 r5, 0x7C496C1F;
imm32 r6, 0x886F374C;
imm32 r7, 0x9E94E279;
A0 = R0;
A1 = R1;
R0.L = ( A0 += A1 );
R0.H = ( A0 += A1 );
R1.L = ( A0 += A1 );
R1.H = ( A0 += A1 );
R2.L = ( A0 += A1 );
R2.H = ( A0 += A1 );
R3.L = ( A0 += A1 );
R3.H = ( A0 += A1 );
R4.L = ( A0 += A1 );
R4.H = ( A0 += A1 );
R5.L = ( A0 += A1 );
R5.H = ( A0 += A1 );
R6.L = ( A0 += A1 );
R6.H = ( A0 += A1 );
R7.L = ( A0 += A1 );
R7.H = ( A0 += A1 );
CHECKREG r0, 0x0BF30940;
CHECKREG r1, 0x11590EA6;
CHECKREG r2, 0x16BE140C;
CHECKREG r3, 0x1C241971;
CHECKREG r4, 0x218A1ED7;
CHECKREG r5, 0x26F0243D;
CHECKREG r6, 0x2C5529A3;
CHECKREG r7, 0x31BB2F08;



pass
