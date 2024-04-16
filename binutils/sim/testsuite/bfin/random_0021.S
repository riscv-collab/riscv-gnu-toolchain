# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x5c604280 | _VS | _AV1S | _AV0S);
	imm32 R3, 0xfe0103fe;
	imm32 R5, 0x1e53cdd8;
	R3.H = R5.L * R3.H (M, IU);
	checkreg R3, 0x800003fe;
	checkreg ASTAT, (0x5c604280 | _VS | _V | _AV1S | _AV0S | _V_COPY);

	dmm32 ASTAT, (0x74a04c00 | _VS | _AV1S | _CC | _AN);
	imm32 R4, 0xfffeffff;
	imm32 R5, 0x174e174e;
	R5.H = R4.L * R5.H (M, IU);
	checkreg R5, 0xe8b2174e;
	checkreg ASTAT, (0x74a04c00 | _VS | _AV1S | _CC | _AN);

	dmm32 ASTAT, (0x34308890 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AN);
	imm32 R3, 0x7fffffff;
	imm32 R4, 0x077b8000;
	imm32 R7, 0x03bd03bd;
	R3.H = R4.L * R7.H (M, IU);
	checkreg R3, 0x8000ffff;
	checkreg ASTAT, (0x34308890 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x58700a90 | _VS | _AV1S | _AC1 | _AQ | _CC | _AN);
	imm32 R1, 0x58978212;
	imm32 R3, 0x62b5775a;
	imm32 R6, 0x4c9c9ee3;
	R6.H = R1.L * R3.L (M, IU);
	checkreg R6, 0x80009ee3;
	checkreg ASTAT, (0x58700a90 | _VS | _V | _AV1S | _AC1 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x40204e00 | _VS | _AV1S | _AV0S | _CC | _AN);
	imm32 R3, 0x297fee00;
	imm32 R5, 0x79aa9d21;
	imm32 R6, 0xfffe7484;
	R6.H = R5.L * R3.L (M, IU);
	checkreg R6, 0x80007484;
	checkreg ASTAT, (0x40204e00 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY | _AN);

	pass
