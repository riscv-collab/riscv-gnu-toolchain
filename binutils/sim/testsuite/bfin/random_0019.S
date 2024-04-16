# Test a few (W32) corner cases
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x68b0ca90 | _VS | _AV1S | _AV0S | _CC | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x70da33ff;
	dmm32 A1.x, 0x0000000f;
	imm32 R0, 0x5e29f819;
	imm32 R1, 0x3f59520b;
	A1 += R0.L * R1.L (M, W32);
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x68b0ca90 | _VS | _AV1S | _AV1 | _AV0S | _CC | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x18300c10 | _VS | _AV1S | _AN);
	dmm32 A0.w, 0x1096b1c1;
	dmm32 A0.x, 0xfffffff1;
	imm32 R6, 0x3a0178ee;
	imm32 R7, 0x17c95e45;
	A0 -= R6.L * R7.L (W32);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x18300c10 | _VS | _AV1S | _AV0S | _AV0 | _AN);

	dmm32 ASTAT, (0x68508800 | _VS | _AV1S | _AV0S | _CC | _AZ);
	dmm32 A0.w, 0x30c8f917;
	dmm32 A0.x, 0xffffffc8;
	imm32 R3, 0x7ad1091c;
	imm32 R4, 0x80002874;
	A0 -= R3.L * R4.L (W32);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x68508800 | _VS | _AV1S | _AV0S | _AV0 | _CC | _AZ);

	dmm32 ASTAT, (0x58708e90 | _VS | _AV0 | _AC1 | _AC0 | _AQ | _AC0_COPY);
	dmm32 A0.w, 0x13de4c3d;
	dmm32 A0.x, 0xffffffa5;
	imm32 R0, 0xf70f956f;
	imm32 R2, 0xf837e08c;
	A0 -= R0.L * R2.H (W32);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x58708e90 | _VS | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _AC0_COPY);

	dmm32 ASTAT, (0x70800280 | _VS | _AV1S | _AC1 | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0x80140410;
	dmm32 A0.x, 0x00000000;
	imm32 R1, 0x028b09a4;
	imm32 R4, 0x00007ffc;
	A0 += R4.L * R1.H (W32);
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x70800280 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x0060c610 | _VS | _AC1 | _AC0 | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x1794b937;
	dmm32 A1.x, 0xfffffff5;
	imm32 R6, 0x008e1c0d;
	A1 -= R6.L * R6.L (W32);
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x0060c610 | _VS | _AV1S | _AV1 | _AC1 | _AC0 | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x2c600410 | _VS | _AV0S | _AC1 | _CC | _AN);
	dmm32 A1.w, 0x2d03ef79;
	dmm32 A1.x, 0x00000079;
	imm32 R5, 0x15d1b500;
	imm32 R6, 0xf7962b39;
	A1 += R6.L * R5.H (W32);
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2c600410 | _VS | _AV1S | _AV1 | _AV0S | _AC1 | _CC | _AN);

	dmm32 ASTAT, (0x5cf04e10 | _VS | _AV0S | _AC1 | _CC | _AC0_COPY);
	dmm32 A0.w, 0x4d50b3f0;
	dmm32 A0.x, 0xfffffffc;
	imm32 R4, 0x6671002a;
	imm32 R7, 0x00288000;
	A0 += R4.L * R7.L (W32);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x5cf04e10 | _VS | _AV0S | _AV0 | _AC1 | _CC | _AC0_COPY);


	dmm32 ASTAT, (0x28908000 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AN);
	dmm32 A1.w, 0xc94e99f1;
	dmm32 A1.x, 0x00000021;
	imm32 R4, 0x7fff52b7;
	imm32 R7, 0x3ebb26c6;
	A1 += R7.L * R4.L (M, W32);
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x28908000 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x34708a00 | _VS | _AV0S | _AQ | _CC | _AC0_COPY);
	dmm32 A1.w, 0xf61f316d;
	dmm32 A1.x, 0x00000061;
	imm32 R1, 0x86f0ffff;
	imm32 R3, 0x791048c5;
	A1 += R1.L * R3.L (M, W32);
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x34708a00 | _VS | _AV1S | _AV1 | _AV0S | _AQ | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x5020c280 | _VS | _V | _AC1 | _AC0 | _V_COPY);
	dmm32 A1.w, 0x8700591f;
	dmm32 A1.x, 0x00000007;
	imm32 R2, 0x145b00b1;
	imm32 R3, 0x7fffffff;
	A1 -= R3.L * R2.H (M, W32);
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x5020c280 | _VS | _V | _AV1S | _AV1 | _AC1 | _AC0 | _V_COPY);

	dmm32 ASTAT, (0x00000290 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);
	dmm32 A0.w, 0xfe84e1ec;
	dmm32 A0.x, 0xffffffff;
	imm32 R1, 0x07e73e7b;
	imm32 R3, 0x00033e7b;
	A0 -= R3.L * R1.H (W32);
	checkreg A0.w, 0xfaa965f2;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x00000290 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x78204a80 | _VS | _AV1S | _CC | _AN);
	dmm32 A0.w, 0xca398210;
	dmm32 A0.x, 0xffffffff;
	imm32 R3, 0xffff0000;
	imm32 R7, 0x00000000;
	A0 += R7.L * R3.L (W32);
	checkreg ASTAT, (0x78204a80 | _VS | _AV1S | _CC | _AN);

	dmm32 ASTAT, (0x04208890 | _VS | _AC1 | _AC0_COPY);
	dmm32 A0.w, 0x224cbaee;
	dmm32 A0.x, 0x00000000;
	imm32 R3, 0x3db86584;
	imm32 R6, 0xdb505ed8;
	A0 -= R6.L * R3.H (W32);
	checkreg A0.w, 0xf491746e;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x04208890 | _VS | _AC1 | _AC0_COPY);

	dmm32 ASTAT, (0x3c908600 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _AC0_COPY);
	dmm32 A0.w, 0x03f7c0ec;
	dmm32 A0.x, 0x00000000;
	imm32 R1, 0x1c25c7b4;
	imm32 R5, 0x3f7da612;
	A0 -= R5.L * R1.L (W32);
	checkreg A0.w, 0xdc6a3b9c;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x3c908600 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _AC0_COPY);

	dmm32 ASTAT, (0x7cb08680 | _VS | _AQ | _CC | _AN);
	dmm32 A0.w, 0xdc7c243c;
	dmm32 A0.x, 0xffffffff;
	imm32 R0, 0xe2ccef4c;
	imm32 R5, 0x7fff8000;
	A0 += R5.L * R0.L (W32);
	checkreg A0.w, 0xed30243c;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x7cb08680 | _VS | _AQ | _CC | _AN);

	dmm32 ASTAT, (0x78f00080 | _VS | _V | _AV1S | _AC1 | _AQ | _V_COPY | _AN);
	dmm32 A0.w, 0x39180f38;
	dmm32 A0.x, 0x00000000;
	imm32 R4, 0x01308ac1;
	imm32 R6, 0x7ffff8fd;
	A0 = R6.L * R4.H (W32);
	checkreg A0.w, 0xffef58e0;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x78f00080 | _VS | _V | _AV1S | _AC1 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x7050c090 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0x010909b0;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x80000000;
	imm32 R6, 0x6ad06150;
	A1 = R6.L * R0.H (W32);
	checkreg A1.w, 0x9eb00000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x7050c090 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x68c04c10 | _VS | _V | _AV0S | _AC0 | _AQ | _V_COPY | _AN);
	dmm32 A0.w, 0x43687862;
	dmm32 A0.x, 0x00000000;
	imm32 R2, 0xff278000;
	imm32 R4, 0x0000436a;
	A0 += R2.L * R4.L (W32);
	checkreg A0.w, 0xfffe7862;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x68c04c10 | _VS | _V | _AV0S | _AC0 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x74a00200 | _AV1 | _AV0S | _AV0 | _AC1 | _V_COPY | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x64c15e6b;
	dmm32 A1.x, 0xffffff87;
	imm32 R4, 0x30b3e20d;
	imm32 R7, 0x4a562069;
	A1 = R4.L * R7.H (M, W32);
	checkreg A1.w, 0xf74db25e;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x74a00200 | _AV0S | _AV0 | _AC1 | _V_COPY | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x70f08410 | _AV0 | _AC1 | _AC0_COPY | _AN | _AZ);
	dmm32 A0.w, 0x5f011b0d;
	dmm32 A0.x, 0xffffff86;
	imm32 R3, 0x21f93a90;
	imm32 R4, 0x1c82d429;
	A0 = R3.H * R4.L (W32);
	checkreg A0.w, 0xf45d49c2;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x70f08410 | _AC1 | _AC0_COPY | _AN | _AZ);

	pass
