# test acc shifts larger than they should be, and ASTAT flags
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x7cc0c090 | _VS | _V | _AV0 | _AC1 | _AQ | _AC0_COPY | _AN | _AZ);
	dmm32 A0.w, 0x1890bdbc;
	dmm32 A0.x, 0x00000079;
	A0 = A0 << 0x2;
	checkreg A0.w, 0x6242f6f0;
	checkreg A0.x, 0xffffffe4;
	checkreg ASTAT, (0x7cc0c090 | _VS | _V | _AC1 | _AQ | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x50508600 | _VS | _V | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x02fe375e;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x21;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0xffffffaf;
	checkreg ASTAT, (0x50508600 | _VS | _V | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x7c800a10 | _VS | _AV0S | _AV0 | _AC1);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	A0 = A0 << 0x1f;
	checkreg ASTAT, (0x7c800a10 | _VS | _AV0S | _AC1 | _AZ);

	dmm32 ASTAT, (0x4440c080 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x2e4b0bba;
	dmm32 A0.x, 0xffffff8c;
	A0 = A0 >> 0x25;
	checkreg A0.w, 0xd0000000;
	checkreg A0.x, 0x0000005d;
	checkreg ASTAT, (0x4440c080 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x4c90c680 | _VS | _AV1S | _AV0S | _AC0 | _CC);
	dmm32 A1.w, 0x3ae26599;
	dmm32 A1.x, 0xfffffff3;
	A1 = A1 >> 0x25;
	checkreg A1.w, 0xc8000000;
	checkreg A1.x, 0x0000002c;
	checkreg ASTAT, (0x4c90c680 | _VS | _AV1S | _AV0S | _AC0 | _CC);

	dmm32 ASTAT, (0x3c204000 | _AV1 | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC);
	dmm32 A1.w, 0x1686a378;
	dmm32 A1.x, 0x0000006a;
	A1 = A1 >> 0x16;
	checkreg A1.w, 0x0001a85a;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x3c204000 | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC);

	dmm32 ASTAT, (0x30500800 | _VS | _AV0S | _AQ);
	dmm32 A1.w, 0x6575285f;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x2e;
	checkreg A1.w, 0xa17c0000;
	checkreg A1.x, 0xffffffd4;
	checkreg ASTAT, (0x30500800 | _VS | _AV0S | _AQ | _AN);

	dmm32 ASTAT, (0x70c04010 | _VS | _AV0S | _AQ | _CC);
	dmm32 A1.w, 0x0c7da4e2;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x29;
	checkreg A1.w, 0x71000000;
	checkreg A1.x, 0xffffffd2;
	checkreg ASTAT, (0x70c04010 | _VS | _AV0S | _AQ | _CC | _AN);

	dmm32 ASTAT, (0x74000600 | _VS | _AC1 | _AQ);
	dmm32 A0.w, 0xd0e47afa;
	dmm32 A0.x, 0x00000006;
	A0 = A0 >> 0x32;
	checkreg A0.w, 0x1ebe8000;
	checkreg A0.x, 0x00000039;
	checkreg ASTAT, (0x74000600 | _VS | _AC1 | _AQ);

	dmm32 ASTAT, (0x4ce08200 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ);
	dmm32 A1.w, 0x1b158860;
	dmm32 A1.x, 0x00000068;
	A1 = A1 >> 0x21;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000030;
	checkreg ASTAT, (0x4ce08200 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ);

	dmm32 ASTAT, (0x48c00610 | _VS | _AV1S | _AQ | _CC | _AN);
	dmm32 A1.w, 0x0a2c41e4;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x25;
	checkreg A1.w, 0x20000000;
	checkreg A1.x, 0x0000000f;
	checkreg ASTAT, (0x48c00610 | _VS | _AV1S | _AQ | _CC);

	dmm32 ASTAT, (0x08700400 | _VS | _V | _AV0S | _AC1 | _CC | _V_COPY | _AZ);
	dmm32 A0.w, 0xec125059;
	dmm32 A0.x, 0xffffffff;
	A0 = A0 >> 0x32;
	checkreg A0.w, 0x94164000;
	checkreg A0.x, 0x00000004;
	checkreg ASTAT, (0x08700400 | _VS | _V | _AV0S | _AC1 | _CC | _V_COPY);

	pass
