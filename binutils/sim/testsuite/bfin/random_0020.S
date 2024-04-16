# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x0cb08810 | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);
	dmm32 A1.w, 0xfcdbede4;
	dmm32 A1.x, 0xffffffff;
		imm32 R5, 0x14c5c1c7;
		imm32 R7, 0x006a5040;
	R5 = (A1 += R7.L * R7.H) (M, IU);
		checkreg R5, 0xfcfd2864;
		checkreg A1.w, 0xfcfd2864;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x0cb08810 | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x6c508a90 | _VS | _V | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x0bcd165c;
	dmm32 A1.x, 0x00000000;
		imm32 R0, 0x439a7ef1;
		imm32 R3, 0x47670015;
		imm32 R6, 0x00008000;
	R3 = (A1 += R6.L * R0.L) (M, IU);
		checkreg R3, 0xcc54965c;
		checkreg A1.w, 0xcc54965c;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x6c508a90 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x38900480 | _VS | _AV0S | _AN);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
		imm32 R1, 0x8000ffff;
		imm32 R3, 0x0000ffff;
		imm32 R6, 0xcb2cf810;
	R3 = (A1 += R6.L * R1.L) (M, IU);
		checkreg R3, 0xf81007f0;
		checkreg A1.w, 0xf81007f0;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x38900480 | _VS | _AV0S | _AN);

	dmm32 ASTAT, (0x20100610 | _VS | _AC1 | _AQ | _AN);
	dmm32 A1.w, 0x36491cf0;
	dmm32 A1.x, 0x00000000;
		imm32 R1, 0x10771108;
		imm32 R2, 0x7fb14fe2;
		imm32 R7, 0x3649ffff;
	R1 = (A1 = R7.L * R2.H) (M, IU);
		checkreg R1, 0xffff804f;
		checkreg A1.w, 0xffff804f;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x20100610 | _VS | _AC1 | _AQ | _AN);

	dmm32 ASTAT, (0x6c304400 | _VS | _AV1S | _AC1 | _AQ);
	dmm32 A1.w, 0xd831c3b7;
	dmm32 A1.x, 0xffffffff;
		imm32 R3, 0x3a98144b;
		imm32 R7, 0xd831c3b7;
	R7 = (A1 -= R3.L * R3.H) (M, IU);
		checkreg R7, 0xd38cb92f;
		checkreg A1.w, 0xd38cb92f;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x6c304400 | _VS | _AV1S | _AC1 | _AQ);

	dmm32 ASTAT, (0x3c50c810 | _VS | _AV1S | _AN | _AZ);
	dmm32 A0.w, 0x13cd1c6c;
	dmm32 A0.x, 0x00000000;
		imm32 R2, 0x4000e935;
		imm32 R3, 0xe0b313cd;
	R3.L = (A0 += R3.H * R2.L) (IU);
		checkreg R3, 0xe0b3ffff;
		checkreg A0.w, 0xe07e8c7b;
		checkreg A0.x, 0x00000000;
		checkreg ASTAT, (0x3c50c810 | _VS | _V | _AV1S | _V_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x7c900280 | _AV1S | _AV0S | _AC1 | _AQ);
	dmm32 A0.w, 0x057e5874;
	dmm32 A0.x, 0x00000000;
		imm32 R0, 0x1c0af520;
		imm32 R6, 0x7caea317;
		imm32 R7, 0x107e8ce4;
	R6.L = (A0 += R7.L * R0.L) (IU);
		checkreg R6, 0x7caeffff;
		checkreg A0.w, 0x8c6628f4;
		checkreg A0.x, 0x00000000;
		checkreg ASTAT, (0x7c900280 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x6cf04090 | _VS | _AV1S | _AV0S | _AC1 | _AZ);
	dmm32 A0.w, 0xdc7d7b8c;
	dmm32 A0.x, 0x00000000;
		imm32 R0, 0x788e00d2;
		imm32 R6, 0x03666070;
	R0.L = (A0 -= R6.H * R6.H) (IU);
		checkreg R0, 0x788effff;
		checkreg A0.w, 0xdc71eee8;
		checkreg A0.x, 0x00000000;
		checkreg ASTAT, (0x6cf04090 | _VS | _V | _AV1S | _AV0S | _AC1 | _V_COPY | _AZ);

	dmm32 ASTAT, (0x4cc04c80 | _VS | _CC);
	dmm32 A1.w, 0x41620ea7;
	dmm32 A1.x, 0x00000057;
		imm32 R1, 0xf611262c;
		imm32 R3, 0x7fff7fff;
		imm32 R4, 0x247ee19c;
	R1 = (A1 += R4.L * R3.L) (IU);
		checkreg R1, 0xffffffff;
		checkreg A1.w, 0xb22f2d0b;
		checkreg A1.x, 0x00000057;
		checkreg ASTAT, (0x4cc04c80 | _VS | _V | _CC | _V_COPY);

	dmm32 ASTAT, (0x28e04610 | _VS | _AV0S | _AC1 | _AC0 | _AN);
	dmm32 A0.w, 0xe1753d16;
	dmm32 A0.x, 0xffffffff;
		imm32 R0, 0x7fffffff;
		imm32 R5, 0x2792ffff;
		imm32 R7, 0xffffd6fa;
	R7.L = (A0 = R0.L * R5.L) (IU);
		checkreg R7, 0xffffffff;
		checkreg A0.w, 0xfffe0001;
		checkreg A0.x, 0x00000000;
		checkreg ASTAT, (0x28e04610 | _VS | _V | _AV0S | _AC1 | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x7c900280 | _AV1S | _AV0S | _AC1 | _AQ);
	dmm32 A0.w, 0x057e5874;
	dmm32 A0.x, 0x00000000;
		imm32 R0, 0x1c0af520;
		imm32 R6, 0x7caea317;
		imm32 R7, 0x107e8ce4;
	R6.L = (A0 += R7.L * R0.L) (IU);
		checkreg R6, 0x7caeffff;
		checkreg A0.w, 0x8c6628f4;
		checkreg A0.x, 0x00000000;
		checkreg ASTAT, (0x7c900280 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x00304000 | _VS | _AV1S | _AQ | _AZ);
	dmm32 A0.w, 0x615bac86;
	dmm32 A0.x, 0x00000000;
		imm32 R2, 0x6d2cbec6;
		imm32 R3, 0xe09db667;
	R3.L = (A0 += R3.H * R2.H) (IU);
		checkreg R3, 0xe09dffff;
		checkreg A0.w, 0xc1252082;
		checkreg A0.x, 0x00000000;
		checkreg ASTAT, (0x00304000 | _VS | _V | _AV1S | _AQ | _V_COPY | _AZ);

	dmm32 ASTAT, (0x5cc00080 | _VS | _AV1S | _AC0 | _CC);
	dmm32 A1.w, 0x70d9985a;
	dmm32 A1.x, 0xffffffd6;
		imm32 R1, 0x8000fdeb;
		imm32 R2, 0x20e07e89;
	R1.H = (A1 += R2.L * R1.L) (M, IU);
		checkreg A1.w, 0xee5b251d;
		checkreg A1.x, 0xffffffd6;
		checkreg ASTAT, (0x5cc00080 | _VS | _V | _AV1S | _AC0 | _CC | _V_COPY);

	dmm32 ASTAT, (0x60e0ce80 | _VS | _AC0 | _AQ | _CC);
	dmm32 A1.w, 0x67798cf6;
	dmm32 A1.x, 0x00000044;
	imm32 R0, 0x00000000;
	imm32 R1, 0x00008e16;
	imm32 R7, 0x00000000;
	R7 = (A1 -= R0.L * R1.L) (M, IU);
	checkreg R7, 0x7fffffff;
	checkreg A1.w, 0x67798cf6;
	checkreg A1.x, 0x00000044;
	checkreg ASTAT, (0x60e0ce80 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x00500210 | _VS | _V | _AV1S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x6f47fe74;
	dmm32 A1.x, 0x00000022;
		imm32 R5, 0x3482aa64;
		imm32 R6, 0x48320cd9;
	R5.H = (A1 -= R6.L * R5.L) (M, IU);
		checkreg R5, 0x7fffaa64;
		checkreg A1.w, 0x66badfb0;
		checkreg A1.x, 0x00000022;
		checkreg ASTAT, (0x00500210 | _VS | _V | _AV1S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x60f04890 | _VS | _V | _AV1S | _AC1 | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x43fdb94f;
	dmm32 A1.x, 0xffffff97;
		imm32 R1, 0x80000000;
		imm32 R7, 0x0f9b234b;
	R1.H = (A1 += R7.L * R1.H) (M, IU);
		checkreg A1.w, 0x55a3394f;
		checkreg A1.x, 0xffffff97;
		checkreg ASTAT, (0x60f04890 | _VS | _V | _AV1S | _AC1 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x60f0c280 | _V | _AV1S | _AV1 | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x33205f9e;
	dmm32 A1.x, 0xfffffffc;
		imm32 R3, 0x39e0545d;
		imm32 R6, 0x0e133731;
	R3 = (A1 -= R3.L * R6.H) (M, IU);
		checkreg R3, 0x80000000;
		checkreg A1.w, 0x2e7d06b7;
		checkreg A1.x, 0xfffffffc;
		checkreg ASTAT, (0x60f0c280 | _VS | _V | _AV1S | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x6c300490 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x2a477a36;
	dmm32 A1.x, 0xfffffff8;
		imm32 R0, 0xff020000;
		imm32 R5, 0x00000000;
		imm32 R7, 0xffff8000;
	R5.H = (A1 -= R0.L * R7.H) (M, IU);
		checkreg R5, 0x80000000;
		checkreg ASTAT, (0x6c300490 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x1400c210 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AN);
	dmm32 A1.w, 0x68033dca;
	dmm32 A1.x, 0xffffffff;
		imm32 R1, 0x00000000;
		imm32 R3, 0x00a36a42;
		imm32 R7, 0x3afd7fff;
	R3.H = (A1 -= R1.L * R7.H) (M, IU);
		checkreg R3, 0x80006a42;
		checkreg ASTAT, (0x1400c210 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x00104810 | _VS | _AV0S | _AC1 | _AQ | _AC0_COPY | _AN);
	dmm32 A1.w, 0xeb4e9a1d;
	dmm32 A1.x, 0xffffff8c;
		imm32 R1, 0xffffec05;
		imm32 R5, 0x80000000;
		imm32 R6, 0x5ffa604a;
	R1.H = (A1 += R6.L * R5.H) (M, IU);
		checkreg R1, 0x8000ec05;
		checkreg A1.w, 0x1b739a1d;
		checkreg A1.x, 0xffffff8d;
		checkreg ASTAT, (0x00104810 | _VS | _V | _AV0S | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x48600280 | _VS | _AV1S | _AV0 | _AC1 | _CC | _AC0_COPY);
	dmm32 A1.w, 0x54463e5f;
	dmm32 A1.x, 0xffffff94;
		imm32 R1, 0x2e0d6820;
		imm32 R4, 0x37855c3d;
		imm32 R6, 0x7b3ca7a0;
	R6.H = (A1 += R4.L * R1.L) (M, IU);
		checkreg R6, 0x8000a7a0;
		checkreg A1.w, 0x79ca8dff;
		checkreg A1.x, 0xffffff94;
		checkreg ASTAT, (0x48600280 | _VS | _V | _AV1S | _AV0 | _AC1 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x3c008480 | _VS | _AV1S | _AC1 | _AC0 | _CC);
	dmm32 A0.w, 0xcdff712a;
	dmm32 A0.x, 0xffffffff;
		imm32 R0, 0x2f3dfc31;
		imm32 R2, 0x1b1a4b4c;
		imm32 R6, 0x7cbed409;
	R2 = (A0 += R6.H * R0.L) (IU);
		checkreg R2, 0xffffffff;
		checkreg A0.w, 0xffffffff;
		checkreg A0.x, 0xffffffff;
		checkreg ASTAT, (0x3c008480 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _CC | _V_COPY);

	dmm32 ASTAT, (0x4ce0ce80 | _VS | _AC1 | _AC0 | _CC);
	dmm32 A0.w, 0xfefe27a4;
	dmm32 A0.x, 0xffffffff;
		imm32 R0, 0x08270055;
		imm32 R1, 0x0000ffc2;
		imm32 R6, 0x5ca7213b;
	R6.L = (A0 += R1.L * R0.H) (IU);
		checkreg R6, 0x5ca7ffff;
		checkreg A0.w, 0xffffffff;
		checkreg A0.x, 0xffffffff;
		checkreg ASTAT, (0x4ce0ce80 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _CC | _V_COPY);

	dmm32 ASTAT, (0x7020ca10 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY);
	dmm32 A0.w, 0xec60b144;
	dmm32 A0.x, 0xffffffff;
		imm32 R0, 0x147e9190;
		imm32 R1, 0x2b813e9e;
		imm32 R4, 0xab65ffff;
	R0 = (A0 += R1.L * R4.H) (IU);
		checkreg R0, 0xffffffff;
		checkreg A0.w, 0xffffffff;
		checkreg A0.x, 0xffffffff;
		checkreg ASTAT, (0x7020ca10 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x28e08210 | _VS | _AQ | _AN);
	dmm32 A0.w, 0xe650ec98;
	dmm32 A0.x, 0xffffffff;
		imm32 R1, 0xcca1b6ef;
		imm32 R2, 0xd762b783;
		imm32 R3, 0xef34e465;
	R2 = (A0 += R3.L * R1.H) (IU);
		checkreg R2, 0xffffffff;
		checkreg A0.w, 0xffffffff;
		checkreg A0.x, 0xffffffff;
		checkreg ASTAT, (0x28e08210 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x58904e00 | _VS | _AC1 | _AC0 | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0xb84b0e88;
	dmm32 A0.x, 0xffffffff;
		imm32 R0, 0x8367ffff;
		imm32 R1, 0xb6a1af0a;
	R1.L = (A0 += R0.H * R1.H) (IU);
		checkreg R1, 0xb6a1ffff;
		checkreg A0.w, 0xffffffff;
		checkreg A0.x, 0xffffffff;
		checkreg ASTAT, (0x58904e00 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x30900810 | _VS | _AV1S | _AC1 | _AQ | _CC);
	dmm32 A1.w, 0xd0762eff;
	dmm32 A1.x, 0xffffffff;
		imm32 R0, 0x00000000;
		imm32 R1, 0x1d9b7fff;
		imm32 R3, 0xf32bf32b;
	R0.H = (A1 += R1.L * R3.L) (M, IU);
		checkreg R0, 0x7fff0000;
		checkreg A1.w, 0x4a0abbd4;
		checkreg A1.x, 0x00000000;
		checkreg ASTAT, (0x30900810 | _VS | _V | _AV1S | _AC1 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x74408290 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY);
	dmm32 A1.w, 0xf1008000;
	dmm32 A1.x, 0xffffffff;
		imm32 R3, 0x0bb78001;
		imm32 R5, 0x0be78000;
		imm32 R7, 0x17cd9a40;
	R3.H = (A1 += R7.L * R5.L) (M, IU);
		checkreg R3, 0x80008001;
		checkreg A1.w, 0xbe208000;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x74408290 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x40900490 | _VS | _AV1S);
	dmm32 A1.w, 0xa9d97d12;
	dmm32 A1.x, 0xffffffff;
		imm32 R0, 0x4e01ffff;
		imm32 R3, 0x12abdd35;
		imm32 R7, 0xa9d966d6;
	R7.H = (A1 += R0.L * R3.L) (M, IU);
		checkreg R7, 0x800066d6;
		checkreg A1.w, 0xa9d89fdd;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x40900490 | _VS | _V | _AV1S | _V_COPY);

	dmm32 ASTAT, (0x20a04290 | _VS | _V | _AV1S | _AQ | _V_COPY | _AN);
	dmm32 A1.w, 0xe552d880;
	dmm32 A1.x, 0xffffffff;
		imm32 R3, 0xfe6bf901;
		imm32 R5, 0xfae40000;
		imm32 R6, 0x3917f106;
	R5.H = (A1 += R6.L * R3.H) (M, IU);
		checkreg R5, 0x80000000;
		checkreg A1.w, 0xd6708a02;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x20a04290 | _VS | _V | _AV1S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x2050c490 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0xfcd2b056;
	dmm32 A1.x, 0xffffffff;
		imm32 R2, 0xff36c118;
		imm32 R4, 0xfffe0001;
		imm32 R7, 0x7fff00f4;
	R7.H = (A1 += R2.L * R4.H) (M, IU);
		checkreg R7, 0x800000f4;
		checkreg A1.w, 0xbdeb2e26;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x2050c490 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x30708290 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x391f1bbc;
	dmm32 A1.x, 0x0000004d;
		imm32 R3, 0xae387ec2;
		imm32 R4, 0x7fff99ff;
		imm32 R5, 0x46730cf4;
	R5 = (A1 += R4.L * R3.H) (M, IU);
		checkreg R5, 0x7fffffff;
		checkreg A1.w, 0xf3b41d84;
		checkreg A1.x, 0x0000004c;
		checkreg ASTAT, (0x30708290 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x60d00200 | _VS | _AV1S | _CC);
	dmm32 A1.w, 0x002b5780;
	dmm32 A1.x, 0x00000000;
		imm32 R1, 0xa07dffff;
		imm32 R2, 0xf90db994;
		imm32 R4, 0x46150060;
	R2.H = (A1 -= R1.L * R4.L) (M, IU);
		checkreg R2, 0x7fffb994;
		checkreg A1.w, 0x002b57e0;
		checkreg A1.x, 0x00000000;
		checkreg ASTAT, (0x60d00200 | _VS | _V | _AV1S | _CC | _V_COPY);

	dmm32 ASTAT, (0x5c600a80 | _VS | _V | _AV1S | _AV1 | _AV0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0x52768086;
	dmm32 A1.x, 0x00000035;
		imm32 R2, 0x1e89d049;
		imm32 R6, 0x5312dd14;
		imm32 R7, 0x02e3d1f4;
	R7 = (A1 += R2.L * R6.L) (M, IU);
		checkreg R7, 0x7fffffff;
		checkreg A1.w, 0x2941cb3a;
		checkreg A1.x, 0x00000035;
		checkreg ASTAT, (0x5c600a80 | _VS | _V | _AV1S | _AV0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x60908080 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AZ);
	dmm32 A1.w, 0x00005d96;
	dmm32 A1.x, 0x00000000;
		imm32 R1, 0x00006828;
		imm32 R5, 0xfffe5480;
		imm32 R7, 0x40000009;
	R5 = (A1 -= R1.L * R7.H) (M, IU);
		checkreg R5, 0xe5f65d96;
		checkreg A1.w, 0xe5f65d96;
		checkreg A1.x, 0xffffffff;
		checkreg ASTAT, (0x60908080 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x3cb08080 | _VS | _AC1 | _CC | _AC0_COPY | _AZ);
	dmm32 A1.w, 0x8b063fca;
	dmm32 A1.x, 0xffffffa2;
		imm32 R3, 0x5f5b566b;
		imm32 R4, 0x800022e6;
		imm32 R5, 0x741acdad;
	R3 = (A1 += R5.L * R4.L) (M, IU);
		checkreg R3, 0x80000000;
		checkreg A1.w, 0x842a0338;
		checkreg A1.x, 0xffffffa2;
		checkreg ASTAT, (0x3cb08080 | _VS | _V | _AC1 | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x60d08a00 | _VS | _AC0 | _AQ | _AN);
	dmm32 A1.w, 0x54eebd9e;
	dmm32 A1.x, 0x00000000;
		imm32 R5, 0x05fa881c;
		imm32 R7, 0xb0728448;
	R5 = (A1 -= R7.L * R5.L) (M, IU);
		checkreg R5, 0x7fffffff;
		checkreg A1.w, 0x96b605be;
		checkreg A1.x, 0x00000000;
		checkreg ASTAT, (0x60d08a00 | _VS | _V | _AC0 | _AQ | _V_COPY | _AN);

	pass
