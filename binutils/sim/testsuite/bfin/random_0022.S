# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x2090c600 | _VS | _AC1 | _AQ | _CC | _AN);
	dmm32 A0.w, 0xf041e418;
	dmm32 A0.x, 0xffffffff;
	imm32 R4, 0x51296cc2;
	imm32 R7, 0xca05cb74;
	R4.L = (A0 += R7.H * R4.L) (TFU);
	checkreg R4, 0x5129ffff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x2090c600 | _VS | _V | _AV0S | _AV0 | _AC1 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x68508090 | _VS | _AV0S | _AC1 | _AC0_COPY);
	dmm32 A1.w, 0xf934c2ea;
	dmm32 A1.x, 0xffffffff;
	imm32 R0, 0x4c8c85a2;
	imm32 R1, 0x13507fff;
	imm32 R7, 0x1bd0df6a;
	R0.H = (A1 += R7.L * R1.L) (TFU);
	checkreg R0, 0xffff85a2;
	checkreg A1.w, 0xffffffff;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x68508090 | _VS | _V | _AV1S | _AV1 | _AV0S | _AC1 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x54e0c200 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0xed4a5c88;
	dmm32 A0.x, 0xffffffff;
	imm32 R1, 0x1332a428;
	imm32 R4, 0x59fd2452;
	imm32 R6, 0x001fffc3;
	R4.L = (A0 += R1.H * R6.L) (TFU);
	checkreg R4, 0x59fdffff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x54e0c200 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x70500000 | _VS | _AV1S | _AV0S | _AC1 | _CC | _AN);
	dmm32 A0.w, 0xb959adf4;
	dmm32 A0.x, 0xffffffff;
	imm32 R0, 0xffc20000;
	imm32 R4, 0x9b83ffff;
	R0.L = (A0 += R4.L * R4.H) (TFU);
	checkreg R0, 0xffc2ffff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x70500000 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x58f04890 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0xfd1277cc;
	dmm32 A0.x, 0xffffffff;
	imm32 R5, 0xfffdffe2;
	imm32 R7, 0x1a9bcac8;
	R5.L = (A0 += R5.H * R7.L) (TFU);
	checkreg R5, 0xfffdffff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x58f04890 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x2840ce90 | _VS | _AV0S | _AC1 | _AC0 | _AC0_COPY);
	dmm32 A1.w, 0x1543f138;
	dmm32 A1.x, 0xffffffce;
	imm32 R3, 0xf4620000;
	imm32 R4, 0x80008000;
	imm32 R7, 0x0d156000;
	R4.H = (A1 -= R3.L * R7.L) (M, TFU);
	checkreg R4, 0x80008000;
	checkreg A1.w, 0x1543f138;
	checkreg A1.x, 0xffffffce;
	checkreg ASTAT, (0x2840ce90 | _VS | _V | _AV0S | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x04000c90 | _AV0S | _AC0 | _AC0_COPY | _AN);
	dmm32 A1.w, 0x7c7b42a9;
	dmm32 A1.x, 0x00000027;
	imm32 R2, 0x28454c31;
	imm32 R5, 0xf220f1b0;
	imm32 R6, 0x257ab18b;
	R2.H = (A1 -= R5.L * R6.L) (M, TFU);
	checkreg R2, 0x7fff4c31;
	checkreg A1.w, 0x86685819;
	checkreg A1.x, 0x00000027;
	checkreg ASTAT, (0x04000c90 | _VS | _V | _AV0S | _AC0 | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x6810ce80 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x00008000;
	imm32 R6, 0x5857bcbe;
	R6.H = (A1 = R6.L * R0.L) (M, TFU);
	checkreg R6, 0xde5fbcbe;
	checkreg A1.w, 0xde5f0000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x6810ce80 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x78c00c80 | _VS | _V | _AC0 | _V_COPY | _AN);
	dmm32 A1.w, 0x63391186;
	dmm32 A1.x, 0x0000005e;
	imm32 R2, 0x34a8b6ef;
	imm32 R7, 0x7c8142e2;
	R7.H = (A1 = R2.L * R2.H) (M, TFU);
	checkreg R7, 0xf0f842e2;
	checkreg A1.w, 0xf0f898d8;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x78c00c80 | _VS | _AC0 | _AN);

	dmm32 ASTAT, (0x70704410 | _VS | _V | _AV0S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x3fff0001;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0xffffffff;
	imm32 R7, 0x80007fff;
	R7.H = (A1 = R0.L * R7.L) (M, TFU);
	checkreg R7, 0xffff7fff;
	checkreg A1.w, 0xffff8001;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x70704410 | _VS | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x00b08610 | _VS | _V | _AV1S | _AV0S | _AV0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xe75e6c55;
	dmm32 A1.x, 0xffffffff;
	imm32 R1, 0x5073b60d;
	imm32 R3, 0x1c5eecaf;
	R1.H = (A1 = R3.L * R3.H) (M, TFU);
	checkreg R1, 0xfddcb60d;
	checkreg A1.w, 0xfddc0c42;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x00b08610 | _VS | _AV1S | _AV0S | _AV0 | _AQ | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x00304690 | _AV1 | _AV0S | _AV0 | _AQ | _AZ);
	dmm32 A1.w, 0x2ef1b58e;
	dmm32 A1.x, 0xffffffd7;
	imm32 R3, 0x37807856;
	imm32 R4, 0x2cd7d02c;
	imm32 R5, 0x4435ba51;
	R4.H = (A1 -= R3.L * R5.L) (M, TFU);
	checkreg R4, 0x8000d02c;
	checkreg A1.w, 0xd75d2658;
	checkreg A1.x, 0xffffffd6;
	checkreg ASTAT, (0x00304690 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY | _AZ);

	dmm32 ASTAT, (0x74c0c600 | _VS | _AV1 | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0x4325067d;
	dmm32 A1.x, 0xffffffee;
	imm32 R0, 0x35ca7288;
	imm32 R5, 0x5ec6e257;
	R0.H = (A1 += R0.L * R5.H) (M, TFU);
	checkreg R0, 0x80007288;
	checkreg A1.w, 0x6d8b8bad;
	checkreg A1.x, 0xffffffee;
	checkreg ASTAT, (0x74c0c600 | _VS | _V | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x50704690 | _VS | _AQ);
	dmm32 A1.w, 0xd0cea2a8;
	dmm32 A1.x, 0xffffffff;
	imm32 R0, 0x11b4e24e;
	imm32 R2, 0xecd6793c;
	imm32 R7, 0x329c2dd6;
	R0.H = (A1 -= R7.L * R2.L) (M, TFU);
	checkreg R0, 0xbb19e24e;
	checkreg A1.w, 0xbb19be80;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x50704690 | _VS | _AQ);

	dmm32 ASTAT, (0x10d08000 | _VS | _AC1 | _AN);
	dmm32 A1.w, 0x32dd86a1;
	dmm32 A1.x, 0xffffffd7;
	imm32 R1, 0xb2310000;
	imm32 R3, 0xd63992d2;
	imm32 R5, 0x2b93b27f;
	R5.H = (A1 += R3.L * R1.L) (M, TFU);
	checkreg R5, 0x8000b27f;
	checkreg A1.w, 0x32dd86a1;
	checkreg A1.x, 0xffffffd7;
	checkreg ASTAT, (0x10d08000 | _VS | _V | _AC1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x3010c600 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _AQ | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0xf99eabd6;
	dmm32 A1.x, 0xffffffff;
	imm32 R2, 0x0c196618;
	imm32 R5, 0x00008000;
	imm32 R6, 0x6617ffff;
	R5.H = (A1 -= R6.L * R2.L) (M, TFU);
	checkreg R5, 0xf99f8000;
	checkreg A1.w, 0xf99f11ee;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x3010c600 | _VS | _AV1S | _AV0S | _AV0 | _AC0 | _AQ | _AC0_COPY);

	dmm32 ASTAT, (0x30f0ca80 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AN);
	dmm32 A0.w, 0x74ea7d56;
	dmm32 A0.x, 0xffffffff;
	imm32 R0, 0x29abffff;
	imm32 R2, 0xade1ffff;
	imm32 R7, 0x20ada3b8;
	R0.L = (A0 += R2.L * R7.L) (TFU);
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x30f0ca80 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x48608210 | _VS | _V | _AV1S | _AV0S | _CC | _V_COPY | _AN);
	dmm32 A0.w, 0x120f0000;
	dmm32 A0.x, 0xffffffff;
	imm32 R3, 0xfeacf0c4;
	R3.L = (A0 += R3.H * R3.H) (TFU);
	checkreg R3, 0xfeacffff;
	checkreg A0.w, 0xffffffff;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x48608210 | _VS | _V | _AV1S | _AV0S | _AV0 | _CC | _V_COPY | _AN);

	pass
