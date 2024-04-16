# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x5c70c410 | _VS | _V | _AV0S | _AV0 | _AC1 | _V_COPY | _AN);
	dmm32 A1.w, 0xb7cc6ddd;
	dmm32 A1.x, 0x00000004;
	imm32 R3, 0x3f225ae3;
	A1 = ASHIFT A1 BY R3.L;
	checkreg A1.w, 0x00000025;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x5c70c410 | _VS | _V | _AV0S | _AV0 | _AC1 | _V_COPY);

	dmm32 ASTAT, (0x4810ca80 | _AV1S | _AV1 | _AV0S | _AV0 | _AC1 | _AC0 | _V_COPY | _AN);
	dmm32 A1.w, 0x7396e11c;
	dmm32 A1.x, 0xffffffba;
	imm32 R3, 0x6e5f9f48;
	A1 = ASHIFT A1 BY R3.L;
	checkreg A1.w, 0x96e11c00;
	checkreg A1.x, 0x00000073;
	checkreg ASTAT, (0x4810ca80 | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _V_COPY);

	pass
