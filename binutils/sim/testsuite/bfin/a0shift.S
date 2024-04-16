# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

// 0xfffffe371c
	r0 = 0;
	r1 = 0;
	r2 = 0;
	r3 = 0;
	r4 = 0;
	r5 = 0;
	r6 = 0;
	r7 = 0;
	a1 = a0 =0;
	astat = R0;

	R6.L = 0x8000;
	R5.H = 0x8000;

// load acc with values;
	R0.L = 0xc062;
	R0.H = 0xffee;
	A0.w = R0;
	R0.L = 0xc52c;
	A0.x = R0;
	R0.L = 0x8d10;
	R0.H = 0x34c;
	A1.w = R0;
	R0.L = 0xe10c;
	A1.x = R0;
// load regs with values;
	R0.L = 0xe844;
	R0.H = 0x4aba;
	R1.L = 0xa294;
	R1.H = 0x52ea;
	R2.L = 0xafda;
	R2.H = 0x5c32;
// end load regs and acc;
	R0.H = (A1 = R5.L * R6.H), R0.L = (A0 += R5.L * R6.H) (FU);
P0 = ASTAT;
CHECKREG P0, (_VS|_V|_V_COPY);

		CHECKREG R0, 0xffff;
R0 = A1.w
CHECKREG R0, 0;
R0 = A1.x
CHECKREG R0, 0;
R0 = A0.w
CHECKREG R0, 0xffeec062;
R0 = A0.x
CHECKREG R0, 0x2c;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY);
	R4 = R6 +|- R5 , R3 = R6 -|+ R5;
		CHECKREG R3, 0x80008000;
		CHECKREG R4, 0x80008000;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN);
	A1 = R7.L * R2.L (M), A0 -= R7.L * R2.H (IS);
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN);
	R7.H = R1.H * R3.L (TFU);
		CHECKREG R7, 0x29750000;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_AN);
	R7.H = ( A1 -= R2.L * R5.H ), A0 = R2.L * R5.H;
		CHECKREG R7, 0xafda0000;
R0 = A1.w
CHECKREG R0, 0xafda0000;
R0 = A1.x
CHECKREG R0, 0xffffffff;
R0 = A0.w
CHECKREG R0, 0x50260000;
R0 = A0.x
CHECKREG R0, 0x0;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_AN);
	R3 = R7.L * R6.H, R2 = R7.L * R6.H (IS);
		CHECKREG R3, 0;
		CHECKREG R2, 0;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_AN);
	R1.H = (A1 += R7.L * R4.H) (M), R1.L = (A0 = R7.H * R4.H) (FU);
		CHECKREG R1, 0xafda57ed;
		P0 = ASTAT;
R0 = A1.w
CHECKREG R0, 0xafda0000;
R0 = A1.x
CHECKREG R0, 0xffffffff;
R0 = A0.w
CHECKREG R0, 0x57ed0000;
R0 = A0.x
CHECKREG R0, 0x0;
		CHECKREG P0, (_VS|_AN);
	R3 = R6.H * R5.L (FU);
		CHECKREG R3, 0;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_AN);
	R5.H = ( A1 += R3.L * R1.L ) (M), A0 -= R3.H * R1.H (ISS2);
		CHECKREG R5, 0x80000000;
R0 = A1.w
CHECKREG R0, 0xafda0000;
R0 = A1.x
CHECKREG R0, 0xffffffff;
R0 = A0.w
CHECKREG R0, 0x57ed0000;
R0 = A0.x
CHECKREG R0, 0x0;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN);
	R3 = R3 +|- R5 , R6 = R3 -|+ R5 (CO);
		CHECKREG R3, 0x80000000;
		CHECKREG R6, 0x00008000;
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN|_AZ);
	R7 = ( A1 += R4.L * R1.L ) (M),  R6 = ( A0 += R4.L * R1.H );
R0 = A1.w
CHECKREG R0, 0x83e38000;
R0 = A1.x
CHECKREG R0, 0xffffffff;
R0 = A0.w
CHECKREG R0, 0xa8130000;
R0 = A0.x
CHECKREG R0, 0x0;
		CHECKREG R6, 0x7fffffff
		CHECKREG R7, 0x83e38000
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN|_AZ);
	IF CC P2 = R1;
	R2.H = (A1 = R7.L * R5.H) (M), R2.L = (A0 = R7.L * R5.H) (ISS2);
		CHECKREG R2, 0x80007fff
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN|_AZ);
	R3.H = R4.H * R2.H, R3.L = R4.L * R2.L (T);
		CHECKREG R3, 0x7fff8001
		P0 = ASTAT;
		CHECKREG P0, (_VS|_V|_V_COPY|_AN|_AZ);
	R7 = ( A1 = R7.H * R1.H ) (M), A0 -= R7.H * R1.H (FU);
		CHECKREG R7, 0xaabe7c4e
		P0 = ASTAT;
		CHECKREG P0, (_VS|_AV0S|_AV0|_AN|_AZ);
	R0.H = R7.L * R4.H (M), R0.L = R7.L * R4.H (TFU);
		CHECKREG R0, 0x3e273e27
		P0 = ASTAT;
		CHECKREG P0, (_VS|_AV0S|_AV0|_AN|_AZ);
	R5 = ( A1 = R7.L * R7.L ),  R4 = ( A0 -= R7.H * R7.H )  (ISS2);
		CHECKREG R5, 0x78b74f88
		CHECKREG R4, 0xc73635f8
R0 = A1.w
CHECKREG R0, 0x3c5ba7c4;
R0 = A1.x
CHECKREG R0, 0x0;
R0 = A0.w
CHECKREG R0, 0xe39b1afc;
R0 = A0.x
CHECKREG R0, 0xffffffff;
		R0 = ASTAT;
		CHECKREG r0, (_VS|_AV0S|_AZ|_AN);
	A0 = A0 >> 2;
		R0 = ASTAT;
		checkreg r0, (_VS|_AV0S);
		R0 = A0.x;
		DBGA (R0.L, 0x3f);
		R0 = A0.w;
		checkreg r0, 0xF8E6C6BF;

	pass
