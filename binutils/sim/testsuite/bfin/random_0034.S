# Verify sign extension behavior with simultaneous acc additions, and
# verify that no ASTAT bits get changed as a result
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x60304880 | _AV1S | _AC0 | _AN | _AZ);
	dmm32 A0.w, 0x589145b7;
	dmm32 A0.x, 0xffffffee;
	dmm32 A1.w, 0x0b247b05;
	dmm32 A1.x, 0x0000005a;
	imm32 R3, 0x1e414332;
	imm32 R4, 0x351715b7;
	R3 = A1.L + A1.H, R4 = A0.L + A0.H;
	checkreg R3, 0x00008629;
	checkreg R4, 0x00009e48;
	checkreg ASTAT, (0x60304880 | _AV1S | _AC0 | _AN | _AZ);

	dmm32 ASTAT, (0x40e0cc00 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0xb2c58001;
	dmm32 A0.x, 0x00000000;
	dmm32 A1.w, 0xe999dc28;
	dmm32 A1.x, 0xffffffff;
	imm32 R0, 0xe58d5ffa;
	imm32 R4, 0x7fff7fff;
	R0 = A1.L + A1.H, R4 = A0.L + A0.H;
	checkreg R0, 0xffffc5c1;
	checkreg R4, 0xffff32c6;
	checkreg ASTAT, (0x40e0cc00 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x3420ca80 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC);
	dmm32 A0.w, 0xeff48350;
	dmm32 A0.x, 0xffffffff;
	dmm32 A1.w, 0x5a3f623a;
	dmm32 A1.x, 0xffffffff;
	imm32 R4, 0xffff152f;
	imm32 R6, 0xdd13218a;
	R4 = A1.L + A1.H, R6 = A0.L + A0.H;
	checkreg R4, 0x0000bc79;
	checkreg R6, 0xffff7344;
	checkreg ASTAT, (0x3420ca80 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC);

	dmm32 ASTAT, (0x10204880 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AN);
	dmm32 A0.w, 0x6da679bb;
	dmm32 A0.x, 0xffffff96;
	dmm32 A1.w, 0x1f5fb024;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x3ebf8000;
	imm32 R6, 0x025f2e8c;
	R6 = A1.L + A1.H, R3 = A0.L + A0.H;
	checkreg R3, 0x0000e761;
	checkreg R6, 0xffffcf83;
	checkreg ASTAT, (0x10204880 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x6ca00c90 | _V | _AV1S | _AV1 | _AC0_COPY | _AN | _AZ);
	dmm32 A0.w, 0x59abaa84;
	dmm32 A0.x, 0xffffffe1;
	dmm32 A1.w, 0x71541efe;
	dmm32 A1.x, 0x00000009;
	imm32 R0, 0x2c41e797;
	imm32 R5, 0x7bfa5e8a;
	R0 = A1.L + A1.H, R5 = A0.L + A0.H;
	checkreg R0, 0x00009052;
	checkreg R5, 0x0000042f;
	checkreg ASTAT, (0x6ca00c90 | _V | _AV1S | _AV1 | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x1c50c290 | _VS | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _AN);
	dmm32 A0.w, 0xffffffff;
	dmm32 A0.x, 0xffffffff;
	dmm32 A1.w, 0xc49ca8db;
	dmm32 A1.x, 0xffffffff;
	imm32 R3, 0x0f62ffff;
	imm32 R4, 0x09505188;
	R4 = A1.L + A1.H, R3 = A0.L + A0.H;
	checkreg R3, 0xfffffffe;
	checkreg R4, 0xffff6d77;
	checkreg ASTAT, (0x1c50c290 | _VS | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _AN);

	dmm32 ASTAT, (0x70e04a90 | _VS | _AV0S | _AQ);
	dmm32 A0.w, 0xd827823e;
	dmm32 A0.x, 0xffffffff;
	dmm32 A1.w, 0x303d11ba;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x80007fff;
	imm32 R6, 0xffc4feb3;
	R6 = A1.L + A1.H, R1 = A0.L + A0.H;
	checkreg R1, 0xffff5a65;
	checkreg R6, 0x000041f7;
	checkreg ASTAT, (0x70e04a90 | _VS | _AV0S | _AQ);

	dmm32 ASTAT, (0x5c80c200 | _VS | _AV0S | _AQ | _AC0_COPY | _AN);
	dmm32 A0.w, 0x97049850;
	dmm32 A0.x, 0x00000000;
	dmm32 A1.w, 0xffffa014;
	dmm32 A1.x, 0xffffffff;
	imm32 R0, 0x04828378;
	imm32 R5, 0x3d9effff;
	R0 = A1.L + A1.H, R5 = A0.L + A0.H;
	checkreg R0, 0xffffa013;
	checkreg R5, 0xffff2f54;
	checkreg ASTAT, (0x5c80c200 | _VS | _AV0S | _AQ | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x6c604600 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AZ);
	dmm32 A0.w, 0xac43c455;
	dmm32 A0.x, 0x00000000;
	dmm32 A1.w, 0x03de6f39;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x5bbfd2d1;
	imm32 R3, 0x22425ebc;
	R3 = A1.L + A1.H, R0 = A0.L + A0.H;
	checkreg R0, 0xffff7098;
	checkreg R3, 0x00007317;
	checkreg ASTAT, (0x6c604600 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AZ);

	dmm32 ASTAT, (0x7cd04280 | _VS | _V | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0xb63ac8f5;
	dmm32 A0.x, 0xffffffe0;
	dmm32 A1.w, 0x358b94e8;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x80007fff;
	imm32 R6, 0x4f4a8883;
	R6 = A1.L + A1.H, R1 = A0.L + A0.H;
	checkreg R1, 0xffff7f2f;
	checkreg R6, 0xffffca73;
	checkreg ASTAT, (0x7cd04280 | _VS | _V | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	pass
