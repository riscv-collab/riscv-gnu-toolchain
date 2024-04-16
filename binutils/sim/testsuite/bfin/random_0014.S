# Test a few corner cases with various shift insns
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x38404290 | _VS | _V | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0xf53d356e;
	dmm32 A0.x, 0xffffffff;
	imm32 R5, 0xaa156b54;
	A0 = ASHIFT A0 BY R5.L;
	checkreg A0.w, 0x56e00000;
	checkreg A0.x, 0xffffffd3;
	checkreg ASTAT, (0x38404290 | _VS | _V | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x28e00410 | _VS | _V | _AV1S | _AV1 | _AC1 | _V_COPY);
	dmm32 A0.w, 0x1dfd2a85;
	dmm32 A0.x, 0xffffffbe;
	imm32 R2, 0x4b7cf707;
	A0 = LSHIFT A0 BY R2.L;
	checkreg A0.w, 0xfe954280;
	checkreg A0.x, 0x0000000e;
	checkreg ASTAT, (0x28e00410 | _VS | _V | _AV1S | _AV1 | _AC1 | _V_COPY);

	dmm32 ASTAT, (0x60404e00 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xd4aa6e10;
	dmm32 A1.x, 0xffffffff;
	imm32 R4, 0xb4bb3054;
	A1 = ASHIFT A1 BY R4.L;
	checkreg A1.w, 0xe1000000;
	checkreg A1.x, 0xffffffa6;
	checkreg ASTAT, (0x60404e00 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x00608810 | _V | _AV1S | _AV0S | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0x0dbadb4f;
	dmm32 A1.x, 0x00000035;
	imm32 R3, 0x3cc3f7db;
	A1 = LSHIFT A1 BY R3.L;
	checkreg A1.w, 0x78000000;
	checkreg A1.x, 0xffffffda;
	checkreg ASTAT, (0x00608810 | _V | _AV1S | _AV0S | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x14900e10 | _VS | _AC0 | _CC | _AC0_COPY);
	imm32 R0, 0x6286ee56;
	imm32 R7, 0x5cd969c5;
	R0 = ASHIFT R0 BY R7.L;
	checkreg R0, 0x50ddcac0;
	checkreg ASTAT, (0x14900e10 | _VS | _V | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x28904a90 | _VS | _V | _AV0S | _V_COPY | _AZ);
	imm32 R0, 0x00000000;
	imm32 R5, 0x00008000;
	imm32 R6, 0x03488f9a;
	R0.L = ASHIFT R5.L BY R6.L;
	checkreg ASTAT, (0x28904a90 | _VS | _V | _AV0S | _V_COPY | _AZ);

	dmm32 ASTAT, (0x3c10c890 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _AC0_COPY);
	imm32 R1, 0x29162006;
	imm32 R3, 0xffff0345;
	imm32 R4, 0x8ff5e6bb;
	R1.H = ASHIFT R4.H BY R3.L;
	checkreg R1, 0xfea02006;
	checkreg ASTAT, (0x3c10c890 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x78600e00 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC);
	imm32 R0, 0xd5b1804d;
	imm32 R1, 0x522c817d;
	imm32 R5, 0xfca6f990;
	R1.H = ASHIFT R5.H BY R0.L;
	checkreg R1, 0xc000817d;
	checkreg ASTAT, (0x78600e00 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x64b04890 | _VS | _V | _AV0S | _AV0 | _AC1 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R4, 0x80000000;
	imm32 R6, 0x4e840a3e;
	imm32 R7, 0x20102e48;
	R6.L = ASHIFT R4.H BY R7.L;
	checkreg R6, 0x4e840000;
	checkreg ASTAT, (0x64b04890 | _VS | _V | _AV0S | _AV0 | _AC1 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);

	pass
