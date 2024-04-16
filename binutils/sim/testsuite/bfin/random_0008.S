# check ASTAT ac/av flags are handled correctly when doing Acc = -Acc
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x5020ca80 | _VS | _AV1S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x549e07b3;
	dmm32 A1.x, 0x0000002a;
	A1 = -A1;
	checkreg A1.w, 0xab61f84d;
	checkreg A1.x, 0xffffffd5;
	checkreg ASTAT, (0x5020ca80 | _VS | _AV1S | _AC0 | _AQ | _CC | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x48908a10 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY | _AN);
	dmm32 A0.w, 0x3c57e100;
	dmm32 A0.x, 0xfffffff2;
	dmm32 A1.w, 0xfb63b8a0;
	dmm32 A1.x, 0xffffffff;
	A1 = -A0;
	checkreg A1.w, 0xc3a81f00;
	checkreg A1.x, 0x0000000d;
	checkreg ASTAT, (0x48908a10 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY);

	dmm32 ASTAT, (0x10900880 | _V | _AC0 | _CC | _AC0_COPY);
	dmm32 A0.w, 0x4ca147ce;
	dmm32 A0.x, 0xffffff9d;
	dmm32 A1.w, 0x0e2534b9;
	dmm32 A1.x, 0xffffff85;
	A0 = -A1;
	checkreg A0.w, 0xf1dacb47;
	checkreg A0.x, 0x0000007a;
	checkreg ASTAT, (0x10900880 | _V | _CC);

	dmm32 ASTAT, (0x34904e90 | _VS | _V | _AV1S | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x7826f07d;
	dmm32 A0.x, 0xffffffc2;
	A0 = -A0;
	checkreg A0.w, 0x87d90f83;
	checkreg A0.x, 0x0000003d;
	checkreg ASTAT, (0x34904e90 | _VS | _V | _AV1S | _AC1 | _V_COPY);

	pass
