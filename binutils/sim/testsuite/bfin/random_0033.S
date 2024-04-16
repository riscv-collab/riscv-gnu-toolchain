# Verify registers saturate and ASTAT bits are updated correctly
# with the RND12 subtract insn
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x24a00410 | _VS | _AV1S | _AV0 | _AC0 | _AC0_COPY | _AN);
	imm32 R5, 0x0fb35119;
	imm32 R6, 0xffffffff;
	imm32 R7, 0x80000000;
	R6.H = R5 - R7 (RND12);
	checkreg R6, 0x7fffffff;
	checkreg ASTAT, (0x24a00410 | _VS | _V | _AV1S | _AV0 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x08c08000 | _VS | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);
	imm32 R3, 0x80003f8f;
	imm32 R5, 0x6267c92c;
	imm32 R6, 0x80000000;
	R5.L = R3 - R6 (RND12);
	checkreg R5, 0x62670004;
	checkreg ASTAT, (0x08c08000 | _VS | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x04200c10 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY);
	imm32 R1, 0x7fff0000;
	imm32 R5, 0x80000000;
	R1.L = R5 - R5 (RND12);
	checkreg ASTAT, (0x04200c10 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _AZ);
	checkreg R1, 0x7fff0000;
	checkreg R5, 0x80000000;

	dmm32 ASTAT, (0x40600e90 | _VS | _AV1S | _AV0S | _AQ | _CC);
	imm32 R1, 0x80000000;
	imm32 R5, 0x00008000;
	imm32 R6, 0x00000000;
	R5.L = R6 - R1 (RND12);
	checkreg R5, 0x00007fff;
	checkreg ASTAT, (0x40600e90 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x68300880 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY | _AZ);
	imm32 R1, 0xf8ed0000;
	imm32 R6, 0x80000000;
	R1.H = R1 - R6 (RND12);
	checkreg R1, 0x7fff0000;
	checkreg ASTAT, (0x68300880 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x70d0c410 | _VS | _AV0S | _AQ);
	imm32 R0, 0x80000000;
	imm32 R1, 0x71455f95;
	imm32 R4, 0xd4871012;
	R4.H = R1 - R0 (RND12);
	checkreg R4, 0x7fff1012;
	checkreg ASTAT, (0x70d0c410 | _VS | _V | _AV0S | _AQ | _V_COPY);

	dmm32 ASTAT, (0x34500e00 | _VS | _AV0S | _AC1 | _CC | _AZ);
	imm32 R2, 0x00000000;
	imm32 R5, 0x00000000;
	imm32 R6, 0x80000000;
	R2.L = R5 - R6 (RND12);
	checkreg R2, 0x00007fff;
	checkreg ASTAT, (0x34500e00 | _VS | _V | _AV0S | _AC1 | _CC | _V_COPY);

	pass
