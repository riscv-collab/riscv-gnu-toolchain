# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x60608a90 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0xf41fbf3f;
	dmm32 A1.x, 0x00000000;
	imm32 R5, 0xd8d95310;
	imm32 R6, 0xd0457fff;
	R5.H = (A1 -= R6.L * R6.H) (M, FU);
	checkreg R5, 0x7fff5310;
	checkreg A1.w, 0x8bfe0f84;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x60608a90 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x54b0ca90 | _VS | _AV1S | _AC1 | _AC0 | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0xf88288c8;
	dmm32 A1.x, 0xffffffff;
	imm32 R0, 0xfffe6736;
	imm32 R2, 0x8000f882;
	imm32 R3, 0xffff8391;
	R0.H = (A1 += R3.L * R2.L) (M, FU);
	checkreg R0, 0x80006736;
	checkreg A1.w, 0x7fb7d06a;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x54b0ca90 | _VS | _V | _AV1S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x1c500480 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x9083dd08;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x00000000;
	imm32 R4, 0x00002492;
	R4.H = (A1 += R4.L * R0.H) (M, FU);
	checkreg R4, 0x7fff2492;
	checkreg ASTAT, (0x1c500480 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x7c00c810 | _AV1S | _AC1 | _AC0);
	dmm32 A1.w, 0x69e86d3f;
	dmm32 A1.x, 0xffffffc2;
	imm32 R1, 0x64f42c5b;
	imm32 R3, 0x4128529d;
	R3 = (A1 -= R3.L * R1.L) (M, FU);
	checkreg R3, 0x80000000;
	checkreg A1.w, 0x5b981370;
	checkreg A1.x, 0xffffffc2;
	checkreg ASTAT, (0x7c00c810 | _VS | _V | _AV1S | _AC1 | _AC0 | _V_COPY);

	dmm32 ASTAT, (0x5cc0c480 | _VS | _AQ | _CC);
	dmm32 A1.w, 0x34bbe964;
	dmm32 A1.x, 0x00000036;
	imm32 R1, 0x7fffffff;
	imm32 R5, 0x7fff427e;
	A1 -= R5.L * R1.L (M, FU);
	checkreg A1.w, 0xf23e2be2;
	checkreg A1.x, 0x00000035;
	checkreg ASTAT, (0x5cc0c480 | _VS | _AQ | _CC);

# here the result is zero, and the _V bit is set
	dmm32 ASTAT, 0x0;
	dmm32 A0.w, 0x00008492;
	dmm32 A0.x, 0x00000000;
	imm32 R2, 0x7fff0002;
	imm32 R3, 0xfa6e8492;
	imm32 R6, 0xffff0002;
	R6 = (A0 -= R3.L * R2.L) (FU);
	checkreg R6, 0x00000000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, ( _VS | _V | _AV0S | _AV0 | _V_COPY);

# here the result is zero, and the _V bit is not set
	dmm32 ASTAT, (_V | _V_COPY);
	dmm32 A0.w, 0x1fffc000;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x80004000;
	imm32 R4, 0x1fffffff;
	imm32 R6, 0x80000000;
	R4.L = (A0 -= R0.L * R6.H) (FU);
	checkreg R4, 0x1fff0000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (_AV0S | _AV0);

	dmm32 ASTAT, (0x0c108610 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY | _AN);
	dmm32 A0.w, 0x0000eaf0;
	dmm32 A0.x, 0x00000000;
	imm32 R1, 0x00010000;
	imm32 R6, 0xfbf10001;
	R1.L = (A0 -= R6.H * R1.H) (FU);
	checkreg R1, 0x00010000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x0c108610 | _VS | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _AC0_COPY | _AN);

	pass
