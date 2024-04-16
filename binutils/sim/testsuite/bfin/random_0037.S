# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x1880c200 | _VS | _AV1S | _AV0S | _AC1);
	dmm32 A0.w, 0x2b9a5661;
	dmm32 A0.x, 0x00000032;
	dmm32 A1.w, 0x1a0c4c8c;
	dmm32 A1.x, 0xffffff80;
	imm32 R0, 0x694a9cb0;
	imm32 R6, 0x651cc0dd;
	A1 += R0.L * R0.H (M), R6.L = (A0 += R0.L * R0.H) (TFU);
	checkreg R6, 0x651cffff;
	checkreg A0.w, 0x6c0bd141;
	checkreg A0.x, 0x00000032;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0xffffff80;
	checkreg ASTAT, (0x1880c200 | _VS | _V | _AV1S | _AV1 | _AV0S | _AC1 | _V_COPY);

	dmm32 ASTAT, (0x14104490 | _VS | _AV1S | _AZ);
	dmm32 A0.w, 0x6ec017a0;
	dmm32 A0.x, 0x00000000;
	dmm32 A1.w, 0xff6f5846;
	dmm32 A1.x, 0x0000007f;
	imm32 R3, 0x256a8306;
	imm32 R6, 0x6a8ca1e4;
	imm32 R7, 0x2e579ce0;
	R6.H = (A1 -= R3.L * R7.L) (M), A0 = R3.L * R7.L (TFU);
	checkreg R6, 0x7fffa1e4;
	checkreg A0.w, 0x504a4d40;
	checkreg A0.x, 0x00000000;
	checkreg A1.w, 0xffffffff;
	checkreg A1.x, 0x0000007f;
	checkreg ASTAT, (0x14104490 | _VS | _V | _AV1S | _AV1 | _V_COPY | _AZ);

	dmm32 ASTAT, (0x20008080 | _VS | _V | _AV1S | _AV0 | _AC1 | _AC0 | _AQ);
	dmm32 A0.w, 0x58b9bdf1;
	dmm32 A0.x, 0xffffffe2;
	dmm32 A1.w, 0x42c9fae8;
	dmm32 A1.x, 0xffffff80;
	imm32 R1, 0x68df1898;
	imm32 R2, 0x3ae1b1f0;
	imm32 R5, 0x61c3f5ef;
	A1 += R2.L * R5.L (M), R1.L = (A0 -= R2.L * R5.L) (TFU);
	checkreg R1, 0x68dfffff;
	checkreg A0.w, 0xadc8eee1;
	checkreg A0.x, 0xffffffe1;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0xffffff80;
	checkreg ASTAT, (0x20008080 | _VS | _V | _AV1S | _AV1 | _AC1 | _AC0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x1c70ca90 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x082c2157;
	dmm32 A0.x, 0xffffff9f;
	dmm32 A1.w, 0x275e1474;
	dmm32 A1.x, 0xffffff80;
	imm32 R1, 0x7d3179fd;
	imm32 R2, 0x5b41566f;
	R2.H = (A1 -= R1.L * R1.H) (M), R2.L = (A0 = R1.L * R1.L) (TFU);
	checkreg R2, 0x80003a21;
	checkreg A0.w, 0x3a212409;
	checkreg A0.x, 0x00000000;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0xffffff80;
	checkreg ASTAT, (0x1c70ca90 | _VS | _V | _AV1S | _AV1 | _AV0S | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x7460cc10 | _VS | _V | _AV1S | _AV0S | _AV0 | _AQ | _CC | _V_COPY);
	dmm32 A0.w, 0x7fffffff;
	dmm32 A0.x, 0x00000000;
	dmm32 A1.w, 0xf20b4000;
	dmm32 A1.x, 0x0000007f;
	imm32 R1, 0x2ca2d045;
	imm32 R6, 0x6e516a3c;
	R1.H = (A1 -= R1.L * R6.H) (M), A0 = R1.L * R6.H (TFU);
	checkreg R1, 0x7fffd045;
	checkreg A0.w, 0x59bf8bd5;
	checkreg A0.x, 0x00000000;
	checkreg A1.w, 0xffffffff;
	checkreg A1.x, 0x0000007f;
	checkreg ASTAT, (0x7460cc10 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _CC | _V_COPY);

	pass
