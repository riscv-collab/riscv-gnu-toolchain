# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x68000a10 | _VS | _V | _AV0S | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x2771851d;
	dmm32 A0.x, 0xffffffc9;
	A0 = A0 >>> 0x1b;
	checkreg A0.w, 0xfffff924;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x68000a10 | _VS | _V | _AV0S | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x74804c10 | _VS | _AC1 | _AC0 | _CC | _AN | _AZ);
	dmm32 A1.w, 0xda2eb5c0;
	dmm32 A1.x, 0xffffffff;
	A1 = A1 >>> 0x11;
	checkreg A1.w, 0xffffed17;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x74804c10 | _VS | _AC1 | _AC0 | _CC | _AN);

	pass
