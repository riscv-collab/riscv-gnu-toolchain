# Test for ASTAT AN setting when overflows occur
# mach: bfin

#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x18204a80 | _AV1S | _AV0 | _AQ | _CC | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x1098e30b;
	dmm32 A1.x, 0x0000001f;
	imm32 R0, 0x440ed6ae;
	imm32 R5, 0x3272c296;
	R0.H = (A1 += R0.L * R5.H);
	checkreg R0, 0x7fffd6ae;
	checkreg A1.w, 0x00500e03;
	checkreg A1.x, 0x0000001f;
	checkreg ASTAT, (0x18204a80 | _VS | _V | _AV1S | _AV0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x28c08e90 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY | _AN);
	dmm32 A0.w, 0xb9da9f02;
	dmm32 A0.x, 0x00000010;
	imm32 R0, 0xc104b252;
	R0.L = A0 (IS);
	checkreg R0, 0xc1047fff;
	checkreg ASTAT, (0x28c08e90 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x3cc04000 | _AV0S | _AV0 | _CC | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x2cc20f30;
	dmm32 A0.x, 0xffffffd0;
	imm32 R2, 0x367adfeb;
	imm32 R5, 0x53eeff3c;
	A0 += R5.H * R2.H (IS);
	checkreg A0.w, 0x3e9e429c;
	checkreg A0.x, 0xffffffd0;
	checkreg ASTAT, (0x3cc04000 | _AV0S | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x18c0ca90 | _V | _AV1S | _AV1 | _AV0 | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x0614ca96;
	dmm32 A1.x, 0x00000053;
	imm32 R3, 0x6c490457;
	R3 = (A1 -= R3.L * R3.L) (M, S2RND);
	checkreg R3, 0x7fffffff;
	checkreg A1.w, 0x0601f505;
	checkreg A1.x, 0x00000053;
	checkreg ASTAT, (0x18c0ca90 | _VS | _V | _AV1S | _AV0 | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN | _AZ);

	pass
