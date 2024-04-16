# Blackfin testcase for ashift
# mach: bfin

	.include "testutils.inc"

	start

	R0 = 5;
	R0 += -1;
	R1 = 4;
	CC = R0 == R1;
	if CC jump 1f;
	fail
1:

	imm32 r2, 0xff901234
	r4=8;
	i2=r2;
	m2 = 4;
	a0 = 0;
	r1.l = (a0 += r4.l *r4.l) (IS) || I2 += m2 || nop;
	r0 = i2;
	imm32 r1, 0xff901238;
	CC = r1 == r0;
	if CC jump 2f;
	fail
2:

	A0 = 0;
	A1 = 0;
	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R2.H = 0xf12e;
	R2.L = 0xbeaa;
	R3.L = 0x00ff;
	A1.w = R2;
	A1.x = R3;
	R0.H = 0xd136;
	R0.L = 0x459d;
	R1.H = 0xabd6;
	R1.L = 0x9ec7;

	R5 = A1 , A0 = R1.L * R0.L (FU);

	R0 = -1 (X);
	CC = r5 == r0;
	if CC jump 3f;
	fail
3:

	R0.L = 0x7bb8;
	R0.H = 0x8d5e;
	R4.L = 0x7e1c;
	R4.H = 0x9e22;
	R6.H = R4.H * R0.L (M), R6.L = R4.L * R0.H (ISS2);

	imm32 r0, 0x80008000
	CC = r6 == r0;
	if CC jump 4f;
	fail
4:
	pass
