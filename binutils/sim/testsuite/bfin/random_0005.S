# Test for ASTAT AZ bit update with 16 bit add and sub insns
# mach: bfin

#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x10a04e10 | _VS | _V | _AV1S | _AQ | _CC | _V_COPY);
	imm32 R3, 0x05fd7405;
	imm32 R7, 0x7fff7fff;
	R3.H = R7.L - R7.H (NS);
	checkreg R3, 0x00007405;
	checkreg ASTAT, (0x10a04e10 | _VS | _AV1S | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x64200e10 | _VS | _AV0S | _AC1 | _AC0 | _AZ);
	imm32 R1, 0x2c388489;
	imm32 R3, 0x38f39dcc;
	imm32 R5, 0x27ed8efa;
	R3.H = R1.L + R5.L (NS);
	checkreg R3, 0x13839dcc;
	checkreg ASTAT, (0x64200e10 | _VS | _V | _AV0S | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	pass
