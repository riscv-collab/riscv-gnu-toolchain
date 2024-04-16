//Original:/testcases/core/c_regmv_dag_lz_dep/c_regmv_dag_lz_dep.dsp
// Spec Reference: regmv dag lz dep forward
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

imm32 r0, 0x11111111;
imm32 r1, 0x22223331;
imm32 r2, 0x44445551;
imm32 r3, 0x66667771;
imm32 r4, 0x88889991;
imm32 r5, 0xaaaabbb1;
imm32 r6, 0xccccddd1;
imm32 r7, 0xeeeefff1;

I0 = R0;
I0 = 0x1122 (Z);
R0 = I0;

I1 = R1;
I1 = 0x3344 (Z);
R1 = I1;

I2 = R2;
I2 = 0x5566 (Z);
R2 = I2;

I3 = R3;
I3 = 0x7788 (Z);
R3 = I3;


B0 = R4;
B0 = 0x99aa (Z);
R4 = B0;

B1 = R5;
B1 = 0xbbcc (Z);
R5 = B1;

B2 = R6;
B2 = 0xddee (Z);
R6 = B2;

B3 = R7;
B3 = 0xff01 (Z);
R7 = B3;

CHECKREG r0, 0x00001122;
CHECKREG r1, 0x00003344;
CHECKREG r2, 0x00005566;
CHECKREG r3, 0x00007788;
CHECKREG r4, 0x000099AA;
CHECKREG r5, 0x0000BBCC;
CHECKREG r6, 0x0000DDEE;
CHECKREG r7, 0x0000FF01;

imm32 r0, 0x11111112;
imm32 r1, 0x22223332;
imm32 r2, 0x44445552;
imm32 r3, 0x66667772;
imm32 r4, 0x88889992;
imm32 r5, 0xaaaabbb2;
imm32 r6, 0xccccddd2;
imm32 r7, 0xeeeefff2;
M0 = R0;
M0 = 0xa1a2 (Z);
R0 = M0;

M1 = R1;
M1 = 0xb1b2 (Z);
R1 = M1;

M2 = R2;
M2 = 0xc1c2 (Z);
R2 = M2;

M3 = R3;
M3 = 0xd1d2 (Z);
R3 = M3;


L0 = R4;
L0 = 0xe1e2 (Z);
R4 = L0;

L1 = R5;
L1 = 0xf1f2 (Z);
R5 = L1;

L2 = R6;
L2 = 0x1112 (Z);
R6 = L2;

L3 = R7;
L3 = 0x2122 (Z);
R7 = L3;

CHECKREG r0, 0x0000A1A2;
CHECKREG r1, 0x0000B1B2;
CHECKREG r2, 0x0000C1C2;
CHECKREG r3, 0x0000D1D2;
CHECKREG r4, 0x0000E1E2;
CHECKREG r5, 0x0000F1F2;
CHECKREG r6, 0x00001112;
CHECKREG r7, 0x00002122;

imm32 r0, 0x11111113;
imm32 r1, 0x22223333;
imm32 r2, 0x44445553;
imm32 r3, 0x66667773;
imm32 r4, 0x88889993;
imm32 r5, 0xaaaabbb3;
imm32 r6, 0xccccddd3;
imm32 r7, 0xeeeefff3;

P1 = R1;
P1 = 0x3A3B (Z);
R1 = P1;


P2 = R2;
P2 = 0x4A4B (Z);
R2 = P2;

P3 = R3;
P3 = 0x5A5B (Z);
R3 = P3;

P4 = R4;
P4 = 0x6A6B (Z);
R4 = P4;

P5 = R5;
P5 = 0x7A7B (Z);
R5 = P5;

CHECKREG r1, 0x00003A3B;
CHECKREG r2, 0x00004A4B;
CHECKREG r3, 0x00005A5B;
CHECKREG r4, 0x00006A6B;
CHECKREG r5, 0x00007A7B;

pass
