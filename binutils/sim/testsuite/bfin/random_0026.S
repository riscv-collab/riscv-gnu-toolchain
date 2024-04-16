# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x4c60c810 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY);
	dmm32 A0.w, 0x7fffffff;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x00000000;
	imm32 R5, 0x00007fff;
	imm32 R7, 0x00000000;
	R7.L = (A0 += R0.L * R5.L) (IH);
	checkreg R7, 0x00007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x4c60c810 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x00500680 | _VS | _AV0S | _AV0 | _AC0 | _AC0_COPY | _AN);
	dmm32 A0.w, 0x80000000;
	dmm32 A0.x, 0xffffffff;
	imm32 R2, 0xffffffff;
	imm32 R4, 0xa8dd8000;
	imm32 R7, 0x80000000;
	R4.L = (A0 -= R2.L * R7.H) (IH);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg R4, 0xa8dd8000;
	checkreg ASTAT, (0x00500680 | _VS | _AV0S | _AV0 | _AC0 | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x50408c90 | _VS | _V | _AV1S | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0xfa400000;
	dmm32 A1.x, 0xffffffad;
	imm32 R0, 0x366b1c84;
	imm32 R3, 0x7fffffff;
	imm32 R7, 0x32528aa5;
	R3.H = (A1 += R0.L * R7.L) (M, IH);
	checkreg R3, 0x8000ffff;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x50408c90 | _VS | _V | _AV1S | _AV1 | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x0c400c10 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xef56cbd3;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x7fff0003;
	imm32 R4, 0x385cffff;
	imm32 R7, 0x680dffff;
	R7.H = (A1 -= R4.L * R3.H) (M, IH);
	checkreg R7, 0x7fffffff;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x0c400c10 | _VS | _V | _AV1S | _AV1 | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x2c604c00 | _AV1S | _AV0 | _AC1);
	dmm32 A1.w, 0xf54ee9bb;
	dmm32 A1.x, 0x0000004a;
	imm32 R3, 0x10bb4bdc;
	imm32 R4, 0x7f29c57d;
	imm32 R7, 0x2c03f00a;
	R4.H = (A1 -= R3.L * R7.H) (M, IH);
	checkreg R4, 0x7fffc57d;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2c604c00 | _VS | _V | _AV1S | _AV1 | _AV0 | _AC1 | _V_COPY);

	dmm32 ASTAT, (0x2c304800 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY);
	dmm32 A1.w, 0xc1a6b608;
	dmm32 A1.x, 0x00000056;
	imm32 R2, 0xd0457fff;
	imm32 R6, 0xf4b2ffff;
	R6.H = (A1 += R2.L * R6.H) (M, IH);
	checkreg R6, 0x7fffffff;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2c304800 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _V_COPY);

	dmm32 ASTAT, (0x04a08810 | _VS | _AV1S | _AC1 | _AC0 | _AN);
	dmm32 A1.w, 0xe9574334;
	dmm32 A1.x, 0x00000056;
	imm32 R3, 0xffffb2bc;
	imm32 R5, 0x03eb4d44;
	imm32 R6, 0x33852750;
	R5.H = (A1 -= R6.L * R3.L) (M, IH);
	checkreg R5, 0x7fff4d44;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x04a08810 | _VS | _V | _AV1S | _AV1 | _AC1 | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x5860c210 | _VS | _V | _AV1S | _AC1 | _AQ | _V_COPY);
	dmm32 A1.w, 0xd5030654;
	dmm32 A1.x, 0x0000001c;
	imm32 R0, 0x20ccb6ee;
	imm32 R2, 0x74c21675;
	imm32 R4, 0x7fff7fff;
	R2.H = (A1 -= R0.L * R4.L) (M, IH);
	checkreg R2, 0x7fff1675;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x5860c210 | _VS | _V | _AV1S | _AV1 | _AC1 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x34800e00 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0xf0b59d3f;
	dmm32 A1.x, 0xffffffef;
	imm32 R4, 0x28bd7772;
	imm32 R6, 0xef66ce6a;
	imm32 R7, 0x80000000;
	R6.H = (A1 -= R4.L * R7.H) (M, IH);
	checkreg R6, 0x8000ce6a;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x34800e00 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x5c804a90 | _VS | _AV1S | _AV0S | _AQ | _AN);
	dmm32 A1.w, 0xc90d8c2f;
	dmm32 A1.x, 0xffffffee;
	imm32 R0, 0x80006a2f;
	imm32 R3, 0x80000000;
	R3.H = (A1 += R0.L * R0.H) (M, IH);
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x5c804a90 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x5c90c010 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0x80ca2186;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0xf3ec0000;
	imm32 R3, 0x5a859a0a;
	imm32 R6, 0x19e852d9;
	R3.H = (A1 -= R1.L * R6.L) (M, IH);
	checkreg R3, 0x7fff9a0a;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x5c90c010 | _VS | _V | _AV1S | _AV1 | _AV0S | _AC0 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x00f00a10 | _VS | _V | _AV0S | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0x9f5baab0;
	dmm32 A1.x, 0x00000019;
	imm32 R1, 0x1bb2489b;
	imm32 R6, 0x0aa80127;
	R1.H = (A1 -= R6.L * R6.H) (M, IH);
	checkreg R1, 0x7fff489b;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x00f00a10 | _VS | _V | _AV1S | _AV1 | _AV0S | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x3c808210 | _VS | _V | _AV1S | _V_COPY | _AN);
	dmm32 A1.w, 0xe09f1e24;
	dmm32 A1.x, 0x00000025;
	imm32 R1, 0x255b55bc;
	imm32 R2, 0x7f1bd115;
	imm32 R3, 0xbc978902;
	R2.H = (A1 -= R3.L * R1.H) (M, IH);
	checkreg R2, 0x7fffd115;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x3c808210 | _VS | _V | _AV1S | _AV1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x1ca04600 | _VS | _V | _AV0S | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0xb80e1ddd;
	dmm32 A1.x, 0xffffffca;
	imm32 R0, 0x2155a4b5;
	imm32 R1, 0x5dd905c2;
	imm32 R2, 0x769083dc;
	R1.H = (A1 -= R2.L * R0.H) (M, IH);
	checkreg R1, 0x800005c2;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x1ca04600 | _VS | _V | _AV1S | _AV1 | _AV0S | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x1cb0cc90 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xfc7c3973;
	dmm32 A1.x, 0xffffff8a;
	imm32 R1, 0x58a6c4e7;
	imm32 R4, 0x19b16033;
	imm32 R6, 0x301ff2ba;
	R6.H = (A1 -= R4.L * R1.H) (M, IH);
	checkreg R6, 0x8000f2ba;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x1cb0cc90 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x2c800810 | _VS | _AV1S | _AQ | _CC | _AN);
	dmm32 A1.w, 0xd86a7676;
	dmm32 A1.x, 0xffffff97;
	imm32 R3, 0x443fea83;
	imm32 R4, 0x47ed4ac3;
	imm32 R6, 0x7fffffff;
	R4.H = (A1 += R3.L * R6.L) (M, IH);
	checkreg R4, 0x80004ac3;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x2c800810 | _VS | _V | _AV1S | _AV1 | _AQ | _CC | _V_COPY | _AN);

	pass
