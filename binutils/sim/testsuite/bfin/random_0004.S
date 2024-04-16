# Test for ASTAT bits being written when they shouldn't (only a reg mov)
# mach: bfin

#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x1c304e90 | _VS | _V | _AV0 | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x74d5f9df;
	dmm32 A0.x, 0x0000005e;
	imm32 R4, 0x00b47e9b;
	R4 = A0;
	checkreg R4, 0x7fffffff;
	checkreg ASTAT, (0x1c304e90 | _VS | _V | _AV0 | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x6cd08a00 | _VS | _AV1 | _AV0S | _AV0 | _AC0 | _AQ | _AN | _AZ);
	dmm32 A1.w, 0x124e2817;
	dmm32 A1.x, 0x00000011;
	imm32 R2, 0x545a7c91;
	R2.H = A1;
	checkreg R2, 0x7fff7c91;
	checkreg ASTAT, (0x6cd08a00 | _VS | _V | _AV1 | _AV0S | _AV0 | _AC0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x60700280 | _V | _AV0S | _AV0 | _AC1 | _V_COPY | _AN);
	dmm32 A0.w, 0x02184a1c;
	dmm32 A0.x, 0xffffffc0;
	imm32 R5, 0x60dc408a;
	R5.L = A0 (IS);
	checkreg R5, 0x60dc8000;
	checkreg ASTAT, (0x60700280 | _VS | _V | _AV0S | _AV0 | _AC1 | _V_COPY | _AN);

	pass
