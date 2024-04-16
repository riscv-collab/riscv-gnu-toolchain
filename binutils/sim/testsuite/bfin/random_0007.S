# Make sure the acc regs are updated even when the search criteria is not met
# (this implicitly affects the top 8 bits)
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x08e00690 | _VS | _AC1 | _AN);
	dmm32 A0.w, 0x42357aea;
	dmm32 A0.x, 0x00000001;
	dmm32 A1.w, 0x3a3f0000;
	dmm32 A1.x, 0x00000000;
	imm32 P0, 0x7119f94d;
	imm32 R4, 0xcdeea690;
	imm32 R5, 0xffb58000;
	imm32 R6, 0x72252b1e;
	(R4, R5) = SEARCH R6 (GE);
	checkreg R4, 0x7119f94d;
	checkreg A0.w, 0x00007aea;
	checkreg A0.x, 0x00000000;
	checkreg A1.w, 0x00007225;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x08e00690 | _VS | _AC1 | _AN);

	dmm32 ASTAT, (0x2cf04210 | _VS | _V | _AC1 | _AC0 | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x4193c6bc;
	dmm32 A0.x, 0xffffffd4;
	dmm32 A1.w, 0xa97e7452;
	dmm32 A1.x, 0xffffffff;
	imm32 P0, 0x51e152a5;
	imm32 R1, 0x36deeb9a;
	imm32 R5, 0x386ab3f7;
	imm32 R7, 0x2a3d5114;
	(R5, R1) = SEARCH R7 (GT);
	checkreg R1, 0x51e152a5;
	checkreg A0.w, 0x00005114;
	checkreg A0.x, 0x00000000;
	checkreg A1.w, 0x00007452;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2cf04210 | _VS | _V | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x2cf04210 | _VS | _V | _AC1 | _AC0 | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x4193c6bc;
	dmm32 A0.x, 0xffffffd4;
	dmm32 A1.w, 0x0000ffff;
	dmm32 A1.x, 0x00000000;
	imm32 P0, 0x51e152a5;
	imm32 R1, 0x36deeb9a;
	imm32 R5, 0x386ab3f7;
	imm32 R7, 0xFa3d5114;
	(R5, R1) = SEARCH R7 (GT);
	checkreg R1, 0x51e152a5;
	checkreg A0.w, 0x00005114;
	checkreg A0.x, 0x00000000;
	checkreg A1.w, 0xffffffff;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x2cf04210 | _VS | _V | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	pass
