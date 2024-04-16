# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x40204090 | _AV1S | _AV0S | _AV0 | _AQ | _AN | _AZ);
	imm32 R1, 0x33e91405;
	imm32 R4, 0x3fa1377c;
	R4.H = R1.H >>> 0x1d;
	checkreg R4, 0x9f48377c;
	checkreg ASTAT, (0x40204090 | _VS | _V | _AV1S | _AV0S | _AV0 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x64800010 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY);
	imm32 R0, 0xf64722bc;
	R0.L = R0.L >>> 0xd (S);
	checkreg R0, 0xf6470001;
	checkreg ASTAT, (0x64800010 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x70300e10 | _VS | _AQ | _AC0_COPY | _AN);
	imm32 R5, 0x2a8771ff;
	R5 = R5 >>> 0x1d (V);
	checkreg R5, 0x54388ff8;
	checkreg ASTAT, (0x70300e10 | _VS | _V | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x04600000 | _VS | _AV1S | _AC0 | _AQ | _CC | _AC0_COPY);
	imm32 R6, 0x0c3a7fff;
	imm32 R7, 0x03460f23;
	R6.H = R7.L >>> 0x1f (S);
	checkreg R6, 0x1e467fff;
	checkreg ASTAT, (0x04600000 | _VS | _AV1S | _AC0 | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x40704010 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R3, 0xfa2cee19;
	imm32 R5, 0xfa2cee17;
	R3.L = R5.H >>> 0xd (S);
	checkreg R3, 0xfa2cffff;
	checkreg ASTAT, (0x40704010 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x20308c90 | _VS | _AV1S | _AV0S | _CC | _AN);
	imm32 R2, 0xd4b70c2f;
	imm32 R5, 0x0279838b;
	R5.H = R2.H >>> 0x1f (S);
	checkreg R5, 0xa96e838b;
	checkreg ASTAT, (0x20308c90 | _VS | _AV1S | _AV0S | _CC | _AN);

	dmm32 ASTAT, (0x10a08690 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	imm32 R4, 0x5cae64fc;
	imm32 R6, 0x288e1461;
	R4.H = R6.L >>> 0x1e (S);
	checkreg R4, 0x518464fc;
	checkreg ASTAT, (0x10a08690 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY);

	dmm32 ASTAT, (0x48908010 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC);
	imm32 R1, 0x4f8f004a;
	imm32 R5, 0x7fff70c1;
	R5.L = R1.L >>> 0x1e (S);
	checkreg R5, 0x7fff0128;
	checkreg ASTAT, (0x48908010 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC);

	dmm32 ASTAT, (0x00f00490 | _VS | _AV0S | _AV0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0x32b127c8;
	dmm32 A0.x, 0x0000001a;
	A0 = A0 >>> 0x6;
	checkreg A0.w, 0x68cac49f;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x00f00490 | _VS | _AV0S | _AQ | _CC | _AC0_COPY);

	pass
