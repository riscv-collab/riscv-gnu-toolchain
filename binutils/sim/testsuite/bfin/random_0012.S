# test VIT_MAX behavior when high Acc bits are set
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x5860c690 | _VS | _AV0S | _AC1 | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0xd81562e8;
	dmm32 A0.x, 0xffffffff;
	imm32 R4, 0x15c2d815;
	imm32 R5, 0xc9bd3a6b;
	R4.L = VIT_MAX (R5) (ASR);
	checkreg R4, 0x15c23a6b;
	checkreg A0.w, 0x6c0ab174;
	checkreg A0.x, 0x0000007f;
	checkreg ASTAT, (0x5860c690 | _VS | _AV0S | _AC1 | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x48308090 | _AV1 | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x715cf6e6;
	dmm32 A0.x, 0xffffffb6;
	imm32 R3, 0x3a89c7ed;
	imm32 R4, 0x4819bbf9;
	R3.L = VIT_MAX (R4) (ASR);
	checkreg R3, 0x3a89bbf9;
	checkreg A0.w, 0x38ae7b73;
	checkreg A0.x, 0x0000005b;
	checkreg ASTAT, (0x48308090 | _AV1 | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x18104c10 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);
	dmm32 A0.w, 0xea06f130;
	dmm32 A0.x, 0xffffffff;
	imm32 R2, 0x62ce98f1;
	imm32 R5, 0x045415f9;
	R2.L = VIT_MAX (R5) (ASR);
	checkreg R2, 0x62ce15f9;
	checkreg A0.w, 0x75037898;
	checkreg A0.x, 0x0000007f;
	checkreg ASTAT, (0x18104c10 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x0090ce10 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0xffffffff;
	dmm32 A0.x, 0xffffffff;
	imm32 R0, 0xc9647fff;
	imm32 R6, 0x1d4baeb8;
	R6.L = VIT_MAX (R0) (ASR);
	checkreg R6, 0x1d4bc964;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0x0000007f;
	checkreg ASTAT, (0x0090ce10 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _CC | _AC0_COPY | _AN);

	pass
