# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x3ce00800 | _VS | _AV1S | _AV0S | _AQ | _AZ);
	imm32 R2, 0x00000000;
	imm32 R4, 0x00000000;
	imm32 R7, 0x00000000;
	R2 = ASHIFT R7 BY R4.L (S);
	checkreg ASTAT, (0x3ce00800 | _VS | _AV1S | _AV0S | _AQ | _AZ);
	checkreg R2, 0x00000000;
	checkreg R4, 0x00000000;
	checkreg R7, 0x00000000;

	dmm32 ASTAT, (0x7c104680 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AZ);
	imm32 R7, 0x00000000;
	R7 = R7 << 0xe (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x7c104680 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x10d08690 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AN);
	imm32 R2, 0x0000ffff;
	imm32 R5, 0x00000000;
	R2 = R5 << 0x1a (S);
	checkreg R2, 0x00000000;
	checkreg ASTAT, (0x10d08690 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x30f08e90 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AN);
	imm32 R6, 0x00000000;
	R6 = ASHIFT R6 BY R6.L (S);
	checkreg ASTAT, (0x30f08e90 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);
	checkreg R6, 0x00000000;

	dmm32 ASTAT, (0x4060c800 | _VS | _AV0S | _AC1 | _CC | _AZ);
	imm32 R5, 0x00000000;
	imm32 R7, 0x00000000;
	R5 = R7 << 0x15 (S);
	checkreg ASTAT, (0x4060c800 | _VS | _AV0S | _AC1 | _CC | _AZ);
	checkreg R5, 0x00000000;
	checkreg R7, 0x00000000;

	dmm32 ASTAT, (0x78604a10 | _VS | _AN);
	imm32 R1, 0x00000000;
	imm32 R4, 0xe1a88000;
	R4 = R1 << 0xb (S);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x78604a10 | _VS | _AZ);

	dmm32 ASTAT, (0x64304800 | _VS | _AV1S | _AV0S | _AC0_COPY);
	imm32 R2, 0x00000000;
	imm32 R7, 0x00000000;
	R7 = R2 << 0xa (S);
	checkreg ASTAT, (0x64304800 | _VS | _AV1S | _AV0S | _AC0_COPY | _AZ);
	checkreg R2, 0x00000000;
	checkreg R7, 0x00000000;

	dmm32 ASTAT, (0x68f0c280 | _VS | _AC1 | _AC0_COPY | _AN);
	imm32 R2, 0x00000000;
	imm32 R5, 0x0000f74a;
	R5 = R2 << 0x10 (S);
	checkreg R5, 0x00000000;
	checkreg ASTAT, (0x68f0c280 | _VS | _AC1 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x54200c80 | _VS | _AV1S | _AV0S | _AQ);
	imm32 R1, 0x00000000;
	imm32 R2, 0x00000000;
	R2 = R1 << 0xa (S);
	checkreg ASTAT, (0x54200c80 | _VS | _AV1S | _AV0S | _AQ | _AZ);
	checkreg R1, 0x00000000;
	checkreg R2, 0x00000000;

	dmm32 ASTAT, (0x20300a80 | _VS | _AV1S | _CC | _AZ);
	imm32 R2, 0x00000000;
	imm32 R7, 0x00000000;
	R7 = R2 << 0x8 (S);
	checkreg ASTAT, (0x20300a80 | _VS | _AV1S | _CC | _AZ);
	checkreg R2, 0x00000000;
	checkreg R7, 0x00000000;

	dmm32 ASTAT, (0x14408e10 | _VS | _AV0S | _AQ | _CC | _AZ);
	imm32 R4, 0x0000007f;
	imm32 R6, 0x00000000;
	R4 = R6 << 0x3 (S);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x14408e10 | _VS | _AV0S | _AQ | _CC | _AZ);

	dmm32 ASTAT, (0x2850c490 | _VS | _AV1S | _AV0S | _AZ);
	imm32 R5, 0x00000000;
	imm32 R7, 0xf67f0000;
	R7 = ASHIFT R5 BY R7.L (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x2850c490 | _VS | _AV1S | _AV0S | _AZ);

	dmm32 ASTAT, (0x24a00400 | _VS | _AV1S | _AC0 | _AC0_COPY | _AN);
	imm32 R4, 0x00001e68;
	imm32 R6, 0x00000000;
	R4 = R6 << 0x8 (S);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x24a00400 | _VS | _AV1S | _AC0 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x34608e00 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _V_COPY | _AN);
	imm32 R1, 0x00000000;
	imm32 R5, 0x272beb60;
	R5 = R1 << 0xa (S);
	checkreg R5, 0x00000000;
	checkreg ASTAT, (0x34608e00 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _AZ);

	dmm32 ASTAT, (0x20800c90 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AN);
	imm32 R3, 0x532993ba;
	imm32 R5, 0x00000000;
	R3 = R5 << 0x9 (S);
	checkreg R3, 0x00000000;
	checkreg ASTAT, (0x20800c90 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x5430c090 | _VS | _V | _AV0S | _AC0 | _AQ | _V_COPY | _AC0_COPY);
	imm32 R1, 0xb1510802;
	imm32 R6, 0x00000000;
	R1 = R6 << 0x1e (S);
	checkreg R1, 0x00000000;
	checkreg ASTAT, (0x5430c090 | _VS | _AV0S | _AC0 | _AQ | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x5cf04c90 | _VS | _AV1S | _AC1 | _AQ | _AC0_COPY);
	dmm32 A1.w, 0xf9bc55b7;
	dmm32 A1.x, 0x0000002a;
	imm32 R0, 0x002d0024;
	imm32 R1, 0x16970042;
	A1 += R0.L * R1.L;
	checkreg A1.w, 0xf9bc6847;
	checkreg A1.x, 0x0000002a;
	checkreg ASTAT, (0x5cf04c90 | _VS | _AV1S | _AC1 | _AQ | _AC0_COPY);

	dmm32 ASTAT, (0x7c804090 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);
	imm32 R5, 0x00000000;
	imm32 R7, 0xfe773828;
	R7 = R5 << 0x19 (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x7c804090 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x30f04e90 | _VS | _AV0S | _AC0 | _AQ);
	imm32 R3, 0x00000000;
	imm32 R7, 0x00000372;
	R7 = R3 << 0x6 (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x30f04e90 | _VS | _AV0S | _AC0 | _AQ | _AZ);

	dmm32 ASTAT, (0x04708210 | _VS | _AV1S | _AC0 | _AQ | _AN);
	imm32 R5, 0x00000000;
	imm32 R7, 0x79b3d220;
	R7 = R5 << 0x13 (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x04708210 | _VS | _AV1S | _AC0 | _AQ | _AZ);

	dmm32 ASTAT, (0x24e08680 | _VS | _AV0S | _AC1 | _CC | _AZ);
	imm32 R0, 0x00000000;
	imm32 R6, 0x00000000;
	imm32 R7, 0xa820afc0;
	R6 = ASHIFT R0 BY R7.L (S);
	checkreg ASTAT, (0x24e08680 | _VS | _AV0S | _AC1 | _CC | _AZ);
	checkreg R0, 0x00000000;
	checkreg R6, 0x00000000;
	checkreg R7, 0xa820afc0;

	dmm32 ASTAT, (0x0ca0c090 | _VS | _AQ | _AZ);
	imm32 R6, 0x00000000;
	imm32 R7, 0x0000001f;
	R7 = R6 << 0x14 (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x0ca0c090 | _VS | _AQ | _AZ);

	dmm32 ASTAT, (0x20204680 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY);
	imm32 R6, 0x00000000;
	R6 = R6 << 0x15 (S);
	checkreg ASTAT, (0x20204680 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY | _AZ);
	checkreg R6, 0x00000000;

	dmm32 ASTAT, (0x14f08c00 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY | _AZ);
	imm32 R2, 0x00000000;
	imm32 R6, 0x00007fff;
	R6 = R2 << 0x1b (S);
	checkreg R6, 0x00000000;
	checkreg ASTAT, (0x14f08c00 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x50b08c00 | _VS | _AC1 | _AQ | _CC | _AN);
	imm32 R1, 0x00000000;
	imm32 R4, 0x0000fffd;
	R4 = R1 << 0x9 (S);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x50b08c00 | _VS | _AC1 | _AQ | _CC | _AZ);

	dmm32 ASTAT, (0x1cb04200 | _VS | _AV0S | _AC1 | _CC);
	imm32 R0, 0x00000000;
	imm32 R2, 0xdeab0000;
	R2 = R0 << 0x1e (S);
	checkreg R2, 0x00000000;
	checkreg ASTAT, (0x1cb04200 | _VS | _AV0S | _AC1 | _CC | _AZ);

	dmm32 ASTAT, (0x54c0ca00 | _VS | _AV1S | _AV0S | _AC1);
	imm32 R6, 0x00000000;
	imm32 R7, 0x9ec9c597;
	R7 = R6 << 0x8 (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x54c0ca00 | _VS | _AV1S | _AV0S | _AC1 | _AZ);

	dmm32 ASTAT, (0x18804400 | _VS | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AN);
	imm32 R7, 0x00000000;
	R7 = R7 << 0x1d (S);
	checkreg ASTAT, (0x18804400 | _VS | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AZ);
	checkreg R7, 0x00000000;

	dmm32 ASTAT, (0x40c08e90 | _VS | _AV1S | _AV0S | _CC);
	imm32 R2, 0x00000000;
	imm32 R5, 0x80000000;
	imm32 R7, 0x00000000;
	R5 = ASHIFT R2 BY R7.L (S);
	checkreg R5, 0x00000000;
	checkreg ASTAT, (0x40c08e90 | _VS | _AV1S | _AV0S | _CC | _AZ);

	dmm32 ASTAT, (0x70b04290 | _VS | _AV1S | _AV0S | _AQ | _AZ);
	imm32 R5, 0x8000c2d0;
	imm32 R6, 0x00000000;
	R5 = R6 << 0x2 (S);
	checkreg R5, 0x00000000;
	checkreg ASTAT, (0x70b04290 | _VS | _AV1S | _AV0S | _AQ | _AZ);

	dmm32 ASTAT, (0x7cf04480 | _VS | _AV0S | _AC0 | _AC0_COPY | _AZ);
	imm32 R3, 0x00000000;
	imm32 R7, 0x00000000;
	R7 = ASHIFT R3 BY R7.L (S);
	checkreg ASTAT, (0x7cf04480 | _VS | _AV0S | _AC0 | _AC0_COPY | _AZ);
	checkreg R3, 0x00000000;
	checkreg R7, 0x00000000;

	dmm32 ASTAT, (0x78d0c290 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);
	imm32 R1, 0x7c98345a;
	imm32 R4, 0x00000000;
	R1 = ASHIFT R4 BY R1.L (S);
	checkreg R1, 0x00000000;
	checkreg ASTAT, (0x78d0c290 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x58400e80 | _VS | _V | _AV0S | _AQ | _CC | _V_COPY);
	imm32 R2, 0x00000000;
	imm32 R4, 0x7fffffff;
	R4 = R2 << 0x8 (S);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x58400e80 | _VS | _AV0S | _AQ | _CC | _AZ);

	dmm32 ASTAT, (0x4c804080 | _VS | _V | _AV1S | _AV0S | _AV0 | _V_COPY);
	imm32 R3, 0x00000000;
	imm32 R7, 0x3d196b66;
	R7 = ASHIFT R3 BY R3.L (S);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x4c804080 | _VS | _AV1S | _AV0S | _AV0 | _AZ);

	dmm32 ASTAT, (0x44304a10 | _VS | _AV0S | _AQ | _AZ);
	imm32 R4, 0x00000000;
	imm32 R6, 0x00000000;
	R6 = R4 << 0x11 (S);
	checkreg ASTAT, (0x44304a10 | _VS | _AV0S | _AQ | _AZ);
	checkreg R4, 0x00000000;
	checkreg R6, 0x00000000;

	pass
