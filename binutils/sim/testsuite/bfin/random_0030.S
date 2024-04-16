# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x00a0cc80 | _VS | _AV1S | _AQ | _CC | _AN);
	dmm32 A1.w, 0x8f7fea28;
	dmm32 A1.x, 0x00000005;
	imm32 R2, 0x000014f2;
	imm32 R4, 0x7fff7fff;
	imm32 R7, 0x14d3a258;
	R7.H = (A1 -= R4.L * R2.H) (M, T);
	checkreg R7, 0x7fffa258;
	checkreg A1.w, 0x8f7fea28;
	checkreg A1.x, 0x00000005;
	checkreg ASTAT, (0x00a0cc80 | _VS | _V | _AV1S | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x7c90c410 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY);
	dmm32 A1.w, 0xbfed6ffc;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x7fffffff;
	imm32 R5, 0x00000000;
	imm32 R6, 0xf70a7fff;
	R0.H = (A1 -= R5.L * R6.L) (M, T);
	checkreg ASTAT, (0x7c90c410 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY);
	checkreg A1.w, 0xbfed6ffc;
	checkreg A1.x, 0x00000000;
	checkreg R0, 0x7fffffff;
	checkreg R5, 0x00000000;
	checkreg R6, 0xf70a7fff;

	dmm32 ASTAT, (0x2c508a10 | _VS | _AV1S | _AV0S | _AC1 | _AQ);
	dmm32 A1.w, 0xfffd8001;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x00018000;
	imm32 R4, 0x7fff8000;
	imm32 R5, 0x7fff0002;
	R3.H = (A1 += R5.L * R4.L) (M, T);
	checkreg R3, 0x7fff8000;
	checkreg A1.w, 0xfffe8001;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2c508a10 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x28408c90 | _VS | _AV1S | _AC0 | _AQ | _AC0_COPY | _AN);
	dmm32 A1.w, 0x842fbc0a;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x04c44422;
	imm32 R3, 0x40f67fff;
	imm32 R7, 0x448c0856;
	R7.H = (A1 -= R3.L * R0.H) (M, T);
	checkreg R7, 0x7fff0856;
	checkreg A1.w, 0x81cdc0ce;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x28408c90 | _VS | _V | _AV1S | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x44708c10 | _AV1S | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0xaa016cf5;
	dmm32 A1.x, 0xffffffdb;
	imm32 R2, 0x25908079;
	imm32 R5, 0x46eabfcd;
	imm32 R7, 0x67066230;
	R2.H = (A1 += R5.L * R7.H) (M, T);
	checkreg R2, 0x80008079;
	checkreg A1.w, 0x902b66c3;
	checkreg A1.x, 0xffffffdb;
	checkreg ASTAT, (0x44708c10 | _VS | _V | _AV1S | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x3c604090 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x8eef28b0;
	dmm32 A1.x, 0x00000023;
	imm32 R0, 0x000156b2;
	imm32 R1, 0xfc1a8000;
	imm32 R5, 0x7fff7fff;
	R5.H = (A1 += R1.L * R0.H) (M, T);
	checkreg A1.w, 0x8eeea8b0;
	checkreg A1.x, 0x00000023;
	checkreg ASTAT, (0x3c604090 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x74208e00 | _VS | _AV0S | _AC0 | _AQ | _AC0_COPY);
	dmm32 A1.w, 0xed3c9973;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x80000000;
	imm32 R1, 0x7fff8000;
	imm32 R2, 0x00000000;
	R1.H = (A1 -= R2.L * R0.H) (M, T);
	checkreg ASTAT, (0x74208e00 | _VS | _V | _AV0S | _AC0 | _AQ | _V_COPY | _AC0_COPY);
	checkreg A1.w, 0xed3c9973;
	checkreg A1.x, 0x00000000;
	checkreg R0, 0x80000000;
	checkreg R1, 0x7fff8000;
	checkreg R2, 0x00000000;

	dmm32 ASTAT, (0x10308800 | _VS | _AV0S | _AC0 | _AC0_COPY);
	dmm32 A1.w, 0x8b345e6e;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0xc40c1663;
	imm32 R4, 0xd0347fff;
	imm32 R7, 0x4249da20;
	R3.H = (A1 += R4.L * R7.H) (M, T);
	checkreg R3, 0x7fff1663;
	checkreg A1.w, 0xac589c25;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x10308800 | _VS | _V | _AV0S | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x1c104880 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY | _AZ);
	dmm32 A1.w, 0xa333ecbc;
	dmm32 A1.x, 0xffffffea;
	imm32 R2, 0x7fffffff;
	imm32 R3, 0x72ea7fff;
	imm32 R4, 0x07348ad1;
	R4.H = (A1 += R2.L * R3.L) (M, T);
	checkreg R4, 0x80008ad1;
	checkreg A1.w, 0xa3336cbd;
	checkreg A1.x, 0xffffffea;
	checkreg ASTAT, (0x1c104880 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY | _AZ);

	dmm32 ASTAT, (0x44904e00 | _VS);
	dmm32 A1.w, 0x90202372;
	dmm32 A1.x, 0xffffffc4;
	imm32 R2, 0x138ac9fc;
	imm32 R3, 0x720a427f;
	imm32 R4, 0x800000f5;
	R3.H = (A1 += R4.L * R2.H) (M, T);
	checkreg R3, 0x8000427f;
	checkreg A1.w, 0x9032d684;
	checkreg A1.x, 0xffffffc4;
	checkreg ASTAT, (0x44904e00 | _VS | _V | _V_COPY);

	dmm32 ASTAT, (0x48f04c90 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0xe9c97364;
	dmm32 A1.x, 0xffffffef;
	imm32 R2, 0x001dffe9;
	imm32 R3, 0x50f06d20;
	imm32 R6, 0x6179b75b;
	R6.H = (A1 -= R3.L * R2.L) (M, T);
	checkreg R6, 0x8000b75b;
	checkreg A1.w, 0x7cb34144;
	checkreg A1.x, 0xffffffef;
	checkreg ASTAT, (0x48f04c90 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x68d00e90 | _VS | _V | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xf3d34812;
	dmm32 A1.x, 0xffffff95;
	imm32 R1, 0xf7419a18;
	imm32 R6, 0x0fdf83b3;
	imm32 R7, 0x0b831070;
	R7.H = (A1 -= R6.L * R1.H) (M, T);
	checkreg R7, 0x80001070;
	checkreg A1.w, 0x6be1229f;
	checkreg A1.x, 0xffffff96;
	checkreg ASTAT, (0x68d00e90 | _VS | _V | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x3060ce80 | _VS | _AV1S | _AC1 | _CC | _AN);
	dmm32 A1.w, 0xe0c1fc60;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x00e97fff;
	imm32 R7, 0x3fff0001;
	R1.H = (A1 += R1.L * R7.H) (M, T);
	checkreg R1, 0x7fff7fff;
	checkreg A1.w, 0x00c13c61;
	checkreg A1.x, 0x00000001;
	checkreg ASTAT, (0x3060ce80 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x3c80c000 | _VS | _AV0S | _AC0_COPY | _AN);
	dmm32 A1.w, 0xb0e43973;
	dmm32 A1.x, 0xffffffbc;
	imm32 R0, 0x511a6fe3;
	imm32 R1, 0x43fe2c80;
	imm32 R2, 0x424b5c19;
	R0.H = (A1 -= R2.L * R1.H) (M, T);
	checkreg R0, 0x80006fe3;
	checkreg A1.w, 0x986e4da5;
	checkreg A1.x, 0xffffffbc;
	checkreg ASTAT, (0x3c80c000 | _VS | _V | _AV0S | _V_COPY | _AC0_COPY | _AN);

	pass
