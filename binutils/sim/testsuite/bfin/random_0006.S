# Test BYTEOP[123]P behavior when source reg pairs match
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	imm32 R0, (0x18204e10 | _VS | _AV1S | _AV0S | _AC1 | _CC);	ASTAT = R0;
	imm32 R1, 0x05b931c4;
	imm32 R4, 0x05f205f2;
	R4 = BYTEOP1P (R1:0, R1:0) (T, R);

	imm32 R0, (0x3470cc10 | _VS | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);	ASTAT = R0;
	imm32 R1, 0x00000000;
	imm32 R6, 0x0000007f;
	R6 = BYTEOP2P (R1:0, R1:0) (RNDH);

	imm32 R0, (0x1c708c90 | _VS | _V | _AV1S | _AC0 | _V_COPY | _AC0_COPY | _AN);	ASTAT = R0;
	imm32 R0, 0x3e2a80ca;
	imm32 R1, 0x20dec740;
	R0 = BYTEOP3P (R1:0, R1:0) (LO);

	pass
