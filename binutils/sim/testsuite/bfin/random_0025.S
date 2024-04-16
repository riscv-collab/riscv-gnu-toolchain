# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x74f00490 | _VS | _V | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R0, 0x10cfffff;
	imm32 R6, 0x06a1ea20;
	R0.H = R6.H >>> 0x1b;
	checkreg R0, 0xd420ffff;
	checkreg ASTAT, (0x74f00490 | _VS | _V | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x78704000 | _VS | _V | _AC0 | _V_COPY);
	imm32 R3, 0x80007fff;
	R3.L = R3.L >>> 0x1f;
	checkreg R3, 0x8000fffe;
	checkreg ASTAT, (0x78704000 | _VS | _V | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x5ce08c00 | _VS | _AV1S | _AQ | _CC | _AC0_COPY | _AN);
	imm32 R3, 0xef9f04f4;
	imm32 R6, 0x11037fff;
	R3.L = R6.H >>> 0x1d;
	checkreg R3, 0xef9f8818;
	checkreg ASTAT, (0x5ce08c00 | _VS | _V | _AV1S | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x14904890 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R2, 0x00af03a2;
	imm32 R7, 0x0b470440;
	R7.L = R2.L >>> 0x1a;
	checkreg R7, 0x0b47e880;
	checkreg ASTAT, (0x14904890 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x3040ca00 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AN);
	imm32 R1, 0x3bd8d8ef;
	imm32 R7, 0x7b15ffff;
	R1.H = R7.H >>> 0x1f;
	checkreg R1, 0xf62ad8ef;
	checkreg ASTAT, (0x3040ca00 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x68404600 | _VS | _V | _AV0S | _AV0 | _V_COPY | _AZ);
	imm32 R0, 0xfffffffc;
	imm32 R1, 0x7ffffffe;
	R0.H = R1.H >>> 0x1f;
	checkreg R0, 0xfffefffc;
	checkreg ASTAT, (0x68404600 | _VS | _V | _AV0S | _AV0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x54108890 | _VS | _V | _AV1S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R1, 0x30b38b8d;
	imm32 R3, 0x1c830bb1;
	R1.H = R3.L >>> 0x1c;
	checkreg R1, 0xbb108b8d;
	checkreg ASTAT, (0x54108890 | _VS | _V | _AV1S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x3cc00e80 | _VS | _AV1S | _AC0);
	imm32 R6, 0x1b42549c;
	R6.L = R6.L >>> 0x1f;
	checkreg R6, 0x1b42a938;
	checkreg ASTAT, (0x3cc00e80 | _VS | _V | _AV1S | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x1ca04490 | _VS | _V | _AV0S | _AC1 | _CC | _V_COPY);
	imm32 R0, 0x0b040a99;
	imm32 R6, 0x2716ffff;
	R6.H = R0.L >>> 0x1c;
	checkreg R6, 0xa990ffff;
	checkreg ASTAT, (0x1ca04490 | _VS | _V | _AV0S | _AC1 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x14800880 | _VS | _AC0 | _AN | _AZ);
	imm32 R2, 0x7fff7fff;
	imm32 R7, 0x0a014f10;
	R7 = R2 >>> 0x1f (V);
	checkreg R7, 0xfffefffe;
	checkreg ASTAT, (0x14800880 | _VS | _V | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x04a08000 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AZ);
	imm32 R7, 0x7fffffff;
	R7 = R7 >>> 0x10 (V);
	checkreg R7, 0x0000ffff;
	checkreg ASTAT, (0x04a08000 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x4c204090 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY);
	imm32 R2, 0x00030003;
	imm32 R6, 0x2c962c96;
	R6 = R2 >>> 0x10 (V);
	checkreg R6, 0x00000000;
	checkreg ASTAT, (0x4c204090 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x14400e00 | _AV1S | _AV1 | _AV0S | _AV0 | _AC1 | _AQ | _AC0_COPY);
	imm32 R0, 0x3a567ee8;
	imm32 R4, 0x7e163337;
	R0 = R4 >>> 0x10 (V);
	checkreg R0, 0x00000000;
	checkreg ASTAT, (0x14400e00 | _AV1S | _AV1 | _AV0S | _AV0 | _AC1 | _AQ | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x30200a10 | _VS | _AN);
	imm32 R2, 0xffff0f44;
	R2 = R2 >>> 0x1c (V);
	checkreg R2, 0xfff0f440;
	checkreg ASTAT, (0x30200a10 | _VS | _V | _V_COPY | _AN);

	dmm32 ASTAT, (0x10c0c080 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);
	imm32 R1, 0x1d4571f3;
	imm32 R2, 0x1d45ffff;
	R2 = R1 >>> 0x10 (V);
	checkreg R2, 0x00000000;
	checkreg ASTAT, (0x10c0c080 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x4cd08c90 | _VS | _AV1S | _AV0S | _CC);
	imm32 R2, 0x8000ffff;
	imm32 R3, 0x0f757fff;
	R3 = R2 >>> 0x10 (V);
	checkreg R3, 0xffffffff;
	checkreg ASTAT, (0x4cd08c90 | _VS | _AV1S | _AV0S | _CC | _AN);

	dmm32 ASTAT, (0x68004a00 | _VS | _AV0S | _AQ | _AN);
	imm32 R6, 0x366a7fff;
	imm32 R7, 0xe4ca366a;
	R7 = R6 >>> 0x1f (V);
	checkreg R7, 0x6cd4fffe;
	checkreg ASTAT, (0x68004a00 | _VS | _V | _AV0S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x14c0ca80 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);
	imm32 R6, 0x3468e405;
	imm32 R7, 0x0fd2ee59;
	R7 = R6 >>> 0x10 (V);
	checkreg R7, 0x0000ffff;
	checkreg ASTAT, (0x14c0ca80 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x1460cc90 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN | _AZ);
	imm32 R3, 0x2b8ffe22;
	imm32 R4, 0x2f17d9d2;
	R4 = R3 >>> 0x1e (V);
	checkreg R4, 0xae3cf888;
	checkreg ASTAT, (0x1460cc90 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x30d04290 | _VS | _AC1 | _AQ | _CC);
	imm32 R1, 0x3afe2bd0;
	imm32 R4, 0x57e37450;
	R4 = R1 >>> 0x10 (V);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x30d04290 | _VS | _AC1 | _AQ | _CC | _AZ);

	dmm32 ASTAT, (0x04600600 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AN);
	imm32 R0, 0xedbbfffe;
	imm32 R4, 0x169330ac;
	R0 = R4 >>> 0x1e (V);
	checkreg R0, 0x5a4cc2b0;
	checkreg ASTAT, (0x04600600 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x64c0c290 | _VS | _AV1 | _AV0S | _AV0 | _AC0 | _CC | _AN);
	imm32 R1, 0x788b2d30;
	imm32 R6, 0x78f61ce9;
	R6 = R1 >>> 0x10 (V);
	checkreg R6, 0x00000000;
	checkreg ASTAT, (0x64c0c290 | _VS | _AV1 | _AV0S | _AV0 | _AC0 | _CC | _AZ);

	dmm32 ASTAT, (0x74d04680 | _VS | _AV0S | _AC0 | _CC | _AC0_COPY);
	imm32 R0, 0x0b7d1dc6;
	imm32 R7, 0x3d27f3e5;
	R7 = R0 >>> 0x10 (V);
	checkreg R7, 0x00000000;
	checkreg ASTAT, (0x74d04680 | _VS | _AV0S | _AC0 | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x74900000 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC);
	imm32 R5, 0xffc70074;
	imm32 R7, 0xf49916ce;
	R5 = R7 >>> 0x10 (V);
	checkreg R5, 0xffff0000;
	checkreg ASTAT, (0x74900000 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC | _AN | _AZ);

	dmm32 ASTAT, (0x6ca0c400 | _VS | _AV0S | _AV0 | _AC0 | _AC0_COPY | _AN);
	imm32 R0, 0x1e0287a7;
	imm32 R4, 0x30aa2286;
	R0 = R4 >>> 0x10 (V);
	checkreg R0, 0x00000000;
	checkreg ASTAT, (0x6ca0c400 | _VS | _AV0S | _AV0 | _AC0 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x10204a00 | _VS | _CC | _AN);
	imm32 R5, 0xa6b04dd0;
	imm32 R6, 0xfedb4cd8;
	R5 = R6 >>> 0x1f (V);
	checkreg R5, 0xfdb699b0;
	checkreg ASTAT, (0x10204a00 | _VS | _V | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x30e04290 | _VS | _V | _AV1S | _AV0S | _AC1 | _V_COPY);
	imm32 R2, 0x0c55766f;
	imm32 R3, 0x28c00004;
	R2 = R3 >>> 0x10 (V);
	checkreg R2, 0x00000000;
	checkreg ASTAT, (0x30e04290 | _VS | _AV1S | _AV0S | _AC1 | _AZ);

	dmm32 ASTAT, (0x34b0c410 | _VS | _AQ | _CC);
	imm32 R7, 0x0f7b2928;
	R7 = R7 >>> 0x1e (V);
	checkreg R7, 0x3deca4a0;
	checkreg ASTAT, (0x34b0c410 | _VS | _V | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x18504800 | _VS | _AV1S | _AC1 | _AC0_COPY);
	imm32 R4, 0x0baad54f;
	imm32 R7, 0x05bf0c50;
	R4 = R7 >>> 0x10 (V);
	checkreg R4, 0x00000000;
	checkreg ASTAT, (0x18504800 | _VS | _AV1S | _AC1 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x2cd04290 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _V_COPY | _AC0_COPY);
	imm32 R0, 0x1199ca48;
	imm32 R7, 0x4ee24366;
	R7 = R0 >>> 0x10 (V);
	checkreg R7, 0x0000ffff;
	checkreg ASTAT, (0x2cd04290 | _VS | _AV1S | _AV0S | _AV0 | _AC0 | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x24304c90 | _VS | _AV0S | _AC1 | _AC0 | _CC);
	imm32 R3, 0x528af4b6;
	imm32 R6, 0x18d26b4a;
	R3 = R6 >>> 0x10 (V);
	checkreg R3, 0x00000000;
	checkreg ASTAT, (0x24304c90 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AZ);

	dmm32 ASTAT, (0x70504200 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AZ);
	imm32 R1, 0x255f0000;
	imm32 R4, 0x96e0e654;
	imm32 R6, 0x255fd442;
	R4 = ASHIFT R1 BY R6.L;
	checkreg R4, 0x957c0000;
	checkreg ASTAT, (0x70504200 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x10004210 | _VS | _AV1S | _AC1 | _AQ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x13f865f4;
	A1 = ASHIFT A1 BY R3.L;
	checkreg ASTAT, (0x10004210 | _VS | _AV1S | _AC1 | _AQ | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R3, 0x13f865f4;

	dmm32 ASTAT, (0x1c90c400 | _VS | _AV0S | _AC1 | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R3, 0x00000000;
	A0 = ASHIFT A0 BY R3.L;
	checkreg ASTAT, (0x1c90c400 | _VS | _AV0S | _AC1 | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R3, 0x00000000;

	dmm32 ASTAT, (0x4820c280 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AN);
	dmm32 A1.w, 0x00000001;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x4a4a7fff;
	A1 = LSHIFT A1 BY R3.L;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x4820c280 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AZ);

	dmm32 ASTAT, (0x1c20cc10 | _VS | _AC1 | _AN);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x00000000;
	A1 = LSHIFT A1 BY R0.L;
	checkreg ASTAT, (0x1c20cc10 | _VS | _AC1 | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R0, 0x00000000;

	dmm32 ASTAT, (0x1c608e90 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R4, 0x10cb0000;
	A0 = ASHIFT A0 BY R4.L;
	checkreg ASTAT, (0x1c608e90 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AC0_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R4, 0x10cb0000;

	dmm32 ASTAT, (0x6870ce00 | _VS | _AC1 | _AC0_COPY | _AZ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R4, 0x00000000;
	A1 = LSHIFT A1 BY R4.L;
	checkreg ASTAT, (0x6870ce00 | _VS | _AC1 | _AC0_COPY | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R4, 0x00000000;

	dmm32 ASTAT, (0x04200290 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R2, 0x00000000;
	A0 = LSHIFT A0 BY R2.L;
	checkreg ASTAT, (0x04200290 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R2, 0x00000000;

	dmm32 ASTAT, (0x0c404e80 | _VS | _V | _V_COPY);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R7, 0xc400e200;
	A0 = ASHIFT A0 BY R7.L;
	checkreg ASTAT, (0x0c404e80 | _VS | _V | _V_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R7, 0xc400e200;

	dmm32 ASTAT, (0x04e00800 | _VS | _AV1S | _AV0S);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0xe603ffff;
	A0 = LSHIFT A0 BY R0.L;
	checkreg ASTAT, (0x04e00800 | _VS | _AV1S | _AV0S | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R0, 0xe603ffff;

	dmm32 ASTAT, (0x40904090 | _VS | _AV0S | _AC1 | _CC | _AZ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R6, 0x00000000;
	A1 = LSHIFT A1 BY R6.L;
	checkreg ASTAT, (0x40904090 | _VS | _AV0S | _AC1 | _CC | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R6, 0x00000000;

	dmm32 ASTAT, (0x24f04c10 | _VS | _V | _AC1 | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x023d0ac0;
	dmm32 A0.x, 0x00000000;
	imm32 R2, 0xfffe05e0;
	A0 = ASHIFT A0 BY R2.L;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x24f04c10 | _VS | _V | _AC1 | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x2860c410 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _AC0_COPY);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R4, 0x00000000;
	A1 = ASHIFT A1 BY R4.L;
	checkreg ASTAT, (0x2860c410 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _AC0_COPY | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R4, 0x00000000;

	dmm32 ASTAT, (0x40000a00 | _VS | _V | _AV0S | _AV0 | _AQ | _CC | _V_COPY | _AN);
	imm32 R2, 0x4e59ffff;
	imm32 R6, 0x2c450001;
	R6 = ASHIFT R2 BY R6.L (V);
	checkreg R6, 0x9cb2fffe;
	checkreg ASTAT, (0x40000a00 | _VS | _V | _AV0S | _AV0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x3c700410 | _VS | _V | _AV0S | _AV0 | _V_COPY | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R6, 0x0d1144c0;
	A0 = LSHIFT A0 BY R6.L;
	checkreg ASTAT, (0x3c700410 | _VS | _V | _AV0S | _V_COPY | _AC0_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R6, 0x0d1144c0;

	dmm32 ASTAT, (0x5c10ca80 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0x80000000;
	dmm32 A1.x, 0x00000000;
	imm32 R7, 0x472d2397;
	A1 = LSHIFT A1 BY R7.L;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x5c10ca80 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x10004c00 | _VS | _AQ | _AZ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R2, 0x80000000;
	A1 = LSHIFT A1 BY R2.L;
	checkreg ASTAT, (0x10004c00 | _VS | _AQ | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R2, 0x80000000;

	dmm32 ASTAT, (0x30308480 | _VS | _AV0S | _AQ);
	dmm32 A0.w, 0x19b289d0;
	dmm32 A0.x, 0x00000000;
	imm32 R6, 0xffff0ce2;
	A0 = LSHIFT A0 BY R6.L;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x30308480 | _VS | _AV0S | _AQ | _AZ);

	dmm32 ASTAT, (0x28708280 | _VS | _AV1S | _AC1 | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0x3f050000;
	dmm32 A0.x, 0x00000000;
	imm32 R6, 0xc0fb081a;
	A0 = LSHIFT A0 BY R6.L;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x28708280 | _VS | _AV1S | _AC1 | _AQ | _CC | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x18708280 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _CC | _AN);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R3, 0xeca83337;
	A0 = LSHIFT A0 BY R3.L;
	checkreg ASTAT, (0x18708280 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _CC | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R3, 0xeca83337;

	dmm32 ASTAT, (0x78b0c010 | _VS | _AV1S | _AC1 | _AC0 | _AN);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R5, 0x00000000;
	A1 = ASHIFT A1 BY R5.L;
	checkreg ASTAT, (0x78b0c010 | _VS | _AV1S | _AC1 | _AC0 | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R5, 0x00000000;

	dmm32 ASTAT, (0x50d00680 | _VS | _AV1S | _AV0S | _AC1 | _AQ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x05600000;
	A1 = LSHIFT A1 BY R3.L;
	checkreg ASTAT, (0x50d00680 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R3, 0x05600000;

	dmm32 ASTAT, (0x04108880 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC);
	dmm32 A0.w, 0x046b40e7;
	dmm32 A0.x, 0x00000000;
	imm32 R3, 0x20a220a2;
	A0 = ASHIFT A0 BY R3.L;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x04108880 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AZ);

	dmm32 ASTAT, (0x6850cc80 | _VS | _AV1S | _AV0S | _AV0 | _AC0_COPY | _AN);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R1, 0x623d1bad;
	A0 = ASHIFT A0 BY R1.L;
	checkreg ASTAT, (0x6850cc80 | _VS | _AV1S | _AV0S | _AC0_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R1, 0x623d1bad;

	dmm32 ASTAT, (0x44d04a80 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY | _AN);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R4, 0x211b1629;
	A1 = LSHIFT A1 BY R4.L;
	checkreg ASTAT, (0x44d04a80 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg R4, 0x211b1629;

	dmm32 ASTAT, (0x1c304480 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R1, 0xffffa0e5;
	A0 = ASHIFT A0 BY R1.L;
	checkreg ASTAT, (0x1c304480 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R1, 0xffffa0e5;

	dmm32 ASTAT, (0x54c00c90 | _VS | _AV0S | _AC1 | _CC | _AZ);
	dmm32 A1.w, 0x01cdbb21;
	dmm32 A1.x, 0x00000000;
	imm32 R7, 0x696f3de3;
	A1 = ASHIFT A1 BY R7.L;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x54c00c90 | _VS | _AV0S | _AC1 | _CC | _AZ);

	dmm32 ASTAT, (0x7c30c690 | _VS | _AV1S | _AV0S | _AC1 | _AC0_COPY | _AN);
	dmm32 A1.w, 0x00007400;
	dmm32 A1.x, 0x00000000;
	imm32 R4, 0x6fc3cc21;
	A1 = LSHIFT A1 BY R4.L;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x7c30c690 | _VS | _AV1S | _AV0S | _AC1 | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x1c404200 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AN);
	imm32 R2, 0x1e000001;
	imm32 R4, 0x037b7038;
	imm32 R5, 0x57beffff;
	R4.L = ASHIFT R5.H BY R2.L;
	checkreg R4, 0x037baf7c;
	checkreg ASTAT, (0x1c404200 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x24e08c80 | _VS | _AV1S | _CC);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R6, 0x11f23024;
	A0 = LSHIFT A0 BY R6.L;
	checkreg ASTAT, (0x24e08c80 | _VS | _AV1S | _CC | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R6, 0x11f23024;

	dmm32 ASTAT, (0x3ce04080 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R2, 0x00000000;
	A0 = ASHIFT A0 BY R2.L;
	checkreg ASTAT, (0x3ce04080 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R2, 0x00000000;

	dmm32 ASTAT, (0x28800280 | _VS | _AV1S | _AV0S | _CC | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x00000000;
	A0 = LSHIFT A0 BY R0.L;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x28800280 | _VS | _AV1S | _AV0S | _CC | _AZ);

	dmm32 ASTAT, (0x68708810 | _VS | _V | _AV1S | _AV0S | _AV1 | _AV0 | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0x80000000;
	dmm32 A1.x, 0xffffffea;
	imm32 R2, 0x0121e8d9;
	A1 = ASHIFT A1 BY R2.L;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x68708810 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x24c00890 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x1b9411f4;
	A1 = LSHIFT A1 BY R0.L;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x24c00890 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x4480ce00 | _VS | _AC1);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	A1 = A1 << 0x5;
	checkreg ASTAT, (0x4480ce00 | _VS | _AC1 | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;

	dmm32 ASTAT, (0x6cf0cc10 | _VS | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x3b;
	checkreg ASTAT, (0x6cf0cc10 | _VS | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;

	dmm32 ASTAT, (0x50d00a80 | _VS | _AV1S | _AV0S | _AC1 | _AN);
	dmm32 A1.w, 0x028ab5f4;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x1f;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x50d00a80 | _VS | _AV1S | _AV0S | _AC1 | _AZ);

	dmm32 ASTAT, (0x14c00490 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x0001f0f0;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >> 0x14;
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x14c00490 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x14808a80 | _VS | _AV1S | _AV0S | _AC1 | _AN);
	dmm32 A0.w, 0x000fc1a6;
	dmm32 A0.x, 0x00000000;
	A0 = A0 >> 0x1f;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x14808a80 | _VS | _AV1S | _AV0S | _AC1 | _AZ);

	dmm32 ASTAT, (0x3c80ca90 | _VS | _AV0S | _AC0 | _AQ | _CC | _AZ);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	A1 = A1 >>> 0x1e;
	checkreg ASTAT, (0x3c80ca90 | _VS | _AV0S | _AC0 | _AQ | _CC | _AZ);
	checkreg A1.w, 0x00000000;
	checkreg A1.x, 0x00000000;

	dmm32 ASTAT, (0x4c200c90 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AQ);
	imm32 R2, 0xf1815f1a;
	imm32 R7, 0x0a917fff;
	R7.L = R2.L >>> 0x13;
	checkreg R7, 0x0a914000;
	checkreg ASTAT, (0x4c200c90 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x0cf0cc80 | _VS | _AV0S | _AC0_COPY | _AZ);
	imm32 R0, 0x000081ad;
	imm32 R2, 0x00000000;
	R2.H = R0.L >>> 0x19;
	checkreg R2, 0xd6800000;
	checkreg ASTAT, (0x0cf0cc80 | _VS | _V | _AV0S | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x04304c10 | _VS | _AV0S | _AC0 | _CC | _AC0_COPY | _AN);
	imm32 R1, 0x33dd7fff;
	imm32 R7, 0xae86a2f4;
	R1 = R7 >>> 0x13 (V);
	checkreg R1, 0xc0008000;
	checkreg ASTAT, (0x04304c10 | _VS | _V | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x7850c800 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AN);
	imm32 R4, 0x0000fffe;
	imm32 R7, 0x5906fc4f;
	R4.L = R7.H >>> 0x15;
	checkreg R4, 0x00003000;
	checkreg ASTAT, (0x7850c800 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _V_COPY);

	dmm32 ASTAT, (0x64804c90 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AN);
	imm32 R1, 0x000009e3;
	imm32 R4, 0x44418b70;
	R1.H = R4.L >>> 0x17;
	checkreg R1, 0xe00009e3;
	checkreg ASTAT, (0x64804c90 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x2c508410 | _VS | _AV1 | _AV0S | _AC0 | _AQ | _AC0_COPY | _AZ);
	imm32 R0, 0x43d731e2;
	imm32 R4, 0x60995f48;
	R0.L = R4.H >>> 0x17;
	checkreg R0, 0x43d73200;
	checkreg ASTAT, (0x2c508410 | _VS | _V | _AV1 | _AV0S | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x0c900010 | _VS | _AV1S | _AV0S | _AC1 | _CC | _AN);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	A0 = A0 >>> 0xc;
	checkreg ASTAT, (0x0c900010 | _VS | _AV1S | _AV0S | _AC1 | _CC | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;

	dmm32 ASTAT, (0x40c00e80 | _VS | _AV1 | _AV0S | _CC | _AN | _AZ);
	imm32 R1, 0x0bf14680;
	imm32 R3, 0x1875266d;
	R3.H = R1.L >>> 0x1d;
	checkreg R3, 0x3400266d;
	checkreg ASTAT, (0x40c00e80 | _VS | _V | _AV1 | _AV0S | _CC | _V_COPY);

	dmm32 ASTAT, (0x78100a00 | _VS | _AV1S | _AV0S | _AC1 | _CC | _AC0_COPY | _AN);
	imm32 R4, 0x67c0a470;
	imm32 R7, 0x000026c0;
	R4 = R7 >>> 0x1d (V);
	checkreg R4, 0x00003600;
	checkreg ASTAT, (0x78100a00 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x6cd04610 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY);
	imm32 R0, 0x0f9535a6;
	imm32 R5, 0x31018b62;
	R0 = R5 >>> 0x12 (V);
	checkreg R0, 0x40008000;
	checkreg ASTAT, (0x6cd04610 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x58a08800 | _VS | _V | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R2, 0x023cffff;
	imm32 R6, 0x0d6d8000;
	R6.L = R2.H >>> 0x18;
	checkreg R6, 0x0d6d3c00;
	checkreg ASTAT, (0x58a08800 | _VS | _V | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x5cc00600 | _VS | _V | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY);
	imm32 R2, 0xa9d7c2fd;
	imm32 R4, 0xfffed266;
	R2.L = R4.L >>> 0x12;
	checkreg R2, 0xa9d78000;
	checkreg ASTAT, (0x5cc00600 | _VS | _V | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x5c900400 | _VS | _AV0S | _AC0 | _CC | _AC0_COPY | _AN);
	imm32 R1, 0xf37e61a8;
	imm32 R4, 0x5522a41c;
	R4 = R1 >>> 0x12 (V);
	checkreg R4, 0x80000000;
	checkreg ASTAT, (0x5c900400 | _VS | _V | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN | _AZ);

	pass
