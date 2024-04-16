# Test logical left shift (vector) insns with larger shift values
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x30400e90 | _VS | _AV0S | _AC1 | _AQ | _AN);
	imm32 R5, 0xb0b40000;
	imm32 R6, 0xf43a5d3c;
	R6 = R5 << 0x19 (V, S);
	checkreg R6, 0xff610000;
	checkreg ASTAT, (0x30400e90 | _VS | _AV0S | _AC1 | _AQ | _AN | _AZ);

	dmm32 ASTAT, (0x34104410 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AN);
	imm32 R2, 0xff2abd08;
	imm32 R5, 0xf610ffff;
	R2 = R5 << 0x11 (V, S);
	checkreg R2, 0xffffffff;
	checkreg ASTAT, (0x34104410 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AN);

	dmm32 ASTAT, (0x6cd0c680 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	imm32 R0, 0x760ecf8e;
	imm32 R1, 0x3f5c8af5;
	R0 = R1 << 0x17 (V, S);
	checkreg R0, 0x001fffc5;
	checkreg ASTAT, (0x6cd0c680 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x44a04280 | _AV1S | _AV1 | _AC1 | _AC0 | _CC);
	imm32 R4, 0x520cb3d4;
	imm32 R6, 0x67141e28;
	R6 = R4 << 0x14 (V, S);
	checkreg R6, 0x0005fffb;
	checkreg ASTAT, (0x44a04280 | _AV1S | _AV1 | _AC1 | _AC0 | _CC | _AN);

	dmm32 ASTAT, (0x14600c10 | _VS | _AV1S | _AC1 | _AC0 | _AN);
	imm32 R3, 0x40407f7e;
	imm32 R4, 0xc081e040;
	R3 = R4 << 0x1a (V, S);
	checkreg R3, 0xff02ff81;
	checkreg ASTAT, (0x14600c10 | _VS | _AV1S | _AC1 | _AC0 | _AN);

	dmm32 ASTAT, (0x04f00490 | _VS | _V | _AV0S | _AC1 | _AQ | _V_COPY);
	imm32 R5, 0x63654235;
	imm32 R7, 0x00008000;
	R5 = R7 << 0x18 (V, S);
	checkreg R5, 0x0000ff80;
	checkreg ASTAT, (0x04f00490 | _VS | _AV0S | _AC1 | _AQ | _AN | _AZ);

	dmm32 ASTAT, (0x3830ca90 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY | _AN);
	imm32 R1, 0x40000000;
	imm32 R2, 0x7fffffff;
	R1 = R2 << 0x16 (V, S);
	checkreg R1, 0x001fffff;
	checkreg ASTAT, (0x3830ca90 | _VS | _AV1S | _AV0S | _AC1 | _CC | _AN);

	dmm32 ASTAT, (0x24e08890 | _VS | _AV0S | _AC1 | _CC | _AN | _AZ);
	imm32 R2, 0xfffe0000;
	imm32 R3, 0xd9d90000;
	R2 = R3 << 0x19 (V, S);
	checkreg R2, 0xffb30000;
	checkreg ASTAT, (0x24e08890 | _VS | _AV0S | _AC1 | _CC | _AN | _AZ);

	dmm32 ASTAT, (0x30f0c200 | _VS | _AV1S | _AQ | _CC | _AC0_COPY | _AZ);
	imm32 R0, 0x32590000;
	imm32 R2, 0x708bb53f;
	R0 = R2 << 0x1c (V, S);
	checkreg R0, 0x0708fb53;
	checkreg ASTAT, (0x30f0c200 | _VS | _AV1S | _AQ | _CC | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x4cc00080 | _VS | _V | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN);
	imm32 R3, 0x3563cfa3;
	imm32 R7, 0x027e2255;
	R7 = R3 << 0x1f (V, S);
	checkreg R7, 0x1ab1e7d1;
	checkreg ASTAT, (0x4cc00080 | _VS | _AC1 | _AQ | _AC0_COPY | _AN);

	pass
