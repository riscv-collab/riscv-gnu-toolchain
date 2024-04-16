# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x3080ca10 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY);
	dmm32 A0.w, 0xee917987;
	dmm32 A0.x, 0x0000007f;
	dmm32 A1.w, 0x116e8678;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x4d56fd82;
	R1.L = (A0 += A1);
	checkreg R1, 0x4d567fff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0x0000007f;
	checkreg ASTAT, (0x3080ca10 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY);

	dmm32 ASTAT, (0x00c04290 | _VS | _V | _AV0S | _V_COPY);
	dmm32 A0.w, 0xe4f8e4c1;
	dmm32 A0.x, 0x0000007f;
	dmm32 A1.w, 0x1b071b3e;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x4b5126c6;
	R1.L = (A0 += A1);
	checkreg R1, 0x4b517fff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0x0000007f;
	checkreg ASTAT, (0x00c04290 | _VS | _V | _AV0S | _V_COPY);

	pass
