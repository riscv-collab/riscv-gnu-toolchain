# Test LSHIFT values and ASTAT flags
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x7ce00000 | _VS | _V | _AV0S | _AQ | _CC | _V_COPY | _AN);
	dmm32 A0.w, 0xe1a3909e;
	dmm32 A0.x, 0xffffffff;
	imm32 R2, 0x214a26f6;
	A0 = LSHIFT A0 BY R2.L;
	checkreg A0.w, 0x3ff868e4;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x7ce00000 | _VS | _V | _AV0S | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x64008a00 | _AV1 | _AV0S | _AV0 | _AC0 | _AQ | _CC | _AN);
	dmm32 A0.w, 0x72af1593;
	dmm32 A0.x, 0xfffffffd;
	imm32 R2, 0x6505b40c;
	A0 = LSHIFT A0 BY R2.L;
	checkreg A0.w, 0xf1593000;
	checkreg A0.x, 0x0000002a;
	checkreg ASTAT, (0x64008a00 | _AV1 | _AV0S | _AC0 | _AQ | _CC);

	pass
