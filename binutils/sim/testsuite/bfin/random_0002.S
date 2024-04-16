# Test for ASTAT V overflows with dsp mult insns
# mach: bfin

#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x54604e00 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY);
	imm32 R1, 0x47f491c5;
	imm32 R3, 0xfe4cfc98;
	imm32 R7, 0x77aa2b21;
	R3.L = R7.H * R1.H (IU);
	checkreg R3, 0xfe4cffff;
	checkreg ASTAT, (0x54604e00 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x10f00200 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AN);
	imm32 R0, 0x24f45737;
	imm32 R1, 0x6752f56b;
	imm32 R4, 0x3f939925;
	R4.H = R0.L * R1.H (IS);
	checkreg R4, 0x7fff9925;
	checkreg ASTAT, (0x10f00200 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AN);

	pass
