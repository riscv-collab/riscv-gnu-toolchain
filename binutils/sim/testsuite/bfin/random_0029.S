# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x2030ca00 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0xdf7ce5c7;
	dmm32 A1.x, 0xffffff9c;
	imm32 R0, 0x098ecb70;
	imm32 R1, 0x80000000;
	R1.H = (A1 += R0.L * R1.H) (M, ISS2);
	checkreg R1, 0x80000000;
	checkreg A1.w, 0xc534e5c7;
	checkreg A1.x, 0xffffff9c;
	checkreg ASTAT, (0x2030ca00 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x00100600 | _VS | _AQ | _AZ);
	dmm32 A1.w, 0xdf39474d;
	dmm32 A1.x, 0xffffffd9;
	imm32 R2, 0x64864b87;
	imm32 R3, 0x61a97f85;
	imm32 R6, 0x1bcacb1a;
	R2.H = (A1 -= R6.L * R3.L) (M, ISS2);
	checkreg R2, 0x80004b87;
	checkreg A1.w, 0xf992dccb;
	checkreg A1.x, 0xffffffd9;
	checkreg ASTAT, (0x00100600 | _VS | _V | _AQ | _V_COPY | _AZ);

	dmm32 ASTAT, (0x50f0c290 | _VS | _AC0 | _AQ | _CC | _AC0_COPY);
	dmm32 A1.w, 0xb0a49eb4;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x1a1607f3;
	imm32 R1, 0x6dcc7fff;
	imm32 R6, 0x80008000;
	R6.H = (A1 -= R1.L * R0.H) (M, ISS2);
	checkreg R6, 0x7fff8000;
	checkreg A1.w, 0xa399b8ca;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x50f0c290 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x48b04c10 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x91b35cde;
	dmm32 A1.x, 0x0000006c;
	imm32 R1, 0xf473c458;
	imm32 R5, 0x1358b0c2;
	imm32 R7, 0xfbf00410;
	R5.H = (A1 -= R1.L * R7.H) (M, ISS2);
	checkreg R5, 0x7fffb0c2;
	checkreg A1.w, 0xcc69025e;
	checkreg A1.x, 0x0000006c;
	checkreg ASTAT, (0x48b04c10 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x1ca04210 | _VS | _AC0 | _AQ | _AN | _AZ);
	dmm32 A1.w, 0xf516677c;
	dmm32 A1.x, 0x00000015;
	imm32 R5, 0x218d4960;
	imm32 R6, 0xfa8c8000;
	R5 = (A1 -= R6.L * R5.H) (M, ISS2);
	checkreg R5, 0x7fffffff;
	checkreg A1.w, 0x05dce77c;
	checkreg A1.x, 0x00000016;
	checkreg ASTAT, (0x1ca04210 | _VS | _V | _AC0 | _AQ | _V_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x04004490 | _VS | _AC1 | _AN);
	dmm32 A1.w, 0xd1795d0a;
	dmm32 A1.x, 0x00000000;
	imm32 R2, 0x67bd270e;
	imm32 R3, 0xda302534;
	imm32 R7, 0x7fffa2af;
	R2.H = (A1 += R7.L * R3.L) (M, ISS2);
	checkreg R2, 0x7fff270e;
	checkreg A1.w, 0xc3e9b396;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x04004490 | _VS | _V | _AC1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x60600490 | _VS | _AV1S | _AC1 | _CC | _AC0_COPY | _AZ);
	dmm32 A1.w, 0xeb8abaea;
	dmm32 A1.x, 0x00000036;
	imm32 R1, 0x111687e8;
	imm32 R5, 0x111687e8;
	R1 = (A1 += R1.L * R5.L) (M, ISS2);
	checkreg R1, 0x7fffffff;
	checkreg A1.w, 0xabc93d2a;
	checkreg A1.x, 0x00000036;
	checkreg ASTAT, (0x60600490 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x30200e80 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _V_COPY | _AN);
	dmm32 A1.w, 0xd3275e78;
	dmm32 A1.x, 0xffffff89;
	imm32 R3, 0xfee80d8d;
	imm32 R6, 0x1c1a8000;
	imm32 R7, 0x00000000;
	R3 = (A1 += R7.L * R6.L) (M, ISS2);
	checkreg R3, 0x80000000;
	checkreg A1.w, 0xd3275e78;
	checkreg A1.x, 0xffffff89;
	checkreg ASTAT, (0x30200e80 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x50208610 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY);
	dmm32 A1.w, 0xb3b71810;
	dmm32 A1.x, 0x00000000;
	imm32 R4, 0xfc2f7ffe;
	imm32 R5, 0x7fffffff;
	imm32 R7, 0x3488c040;
	R7.H = (A1 -= R4.L * R5.H) (M, ISS2);
	checkreg R7, 0x7fffc040;
	checkreg A1.w, 0x73b8980e;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x50208610 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x48d04410 | _VS | _AV1S | _AV0S | _AC0 | _AQ);
	dmm32 A1.w, 0xeb066305;
	dmm32 A1.x, 0xffffff9c;
	imm32 R0, 0x80002105;
	imm32 R4, 0xf4fbe11e;
	imm32 R7, 0xffffb83a;
	R7 = (A1 += R0.L * R4.L) (M, ISS2);
	checkreg R7, 0x80000000;
	checkreg A1.w, 0x080fa69b;
	checkreg A1.x, 0xffffff9d;
	checkreg ASTAT, (0x48d04410 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x3850c090 | _VS | _AV1S | _AV0S | _AC1 | _CC);
	dmm32 A1.w, 0xdfed6537;
	dmm32 A1.x, 0xffffffae;
	imm32 R0, 0xe962c700;
	imm32 R4, 0x32c97fff;
	imm32 R7, 0x28da7373;
	R4.H = (A1 += R7.L * R0.H) (M, ISS2);
	checkreg R4, 0x80007fff;
	checkreg A1.w, 0x492d423d;
	checkreg A1.x, 0xffffffaf;
	checkreg ASTAT, (0x3850c090 | _VS | _V | _AV1S | _AV0S | _AC1 | _CC | _V_COPY);

	dmm32 ASTAT, (0x78a0ce00 | _VS | _AV1S | _AC0 | _AQ | _CC);
	dmm32 A1.w, 0x8c733a78;
	dmm32 A1.x, 0x0000002d;
	imm32 R1, 0x3840acb0;
	imm32 R3, 0x47b843ad;
	imm32 R7, 0x7fff4d00;
	R7 = (A1 += R1.L * R3.H) (M, ISS2);
	checkreg R7, 0x7fffffff;
	checkreg A1.w, 0x751c28f8;
	checkreg A1.x, 0x0000002d;
	checkreg ASTAT, (0x78a0ce00 | _VS | _V | _AV1S | _AC0 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x3cf08880 | _VS | _AV1S | _AV0S | _AC0);
	dmm32 A1.w, 0xbde0b55f;
	dmm32 A1.x, 0xfffffffd;
	imm32 R0, 0x80002300;
	imm32 R5, 0x635db45a;
	imm32 R7, 0x67e67af3;
	R7 = (A1 += R0.L * R5.L) (M, ISS2);
	checkreg R7, 0x80000000;
	checkreg A1.w, 0xd689035f;
	checkreg A1.x, 0xfffffffd;
	checkreg ASTAT, (0x3cf08880 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY);

	dmm32 ASTAT, (0x58608410 | _VS | _AQ | _CC | _AZ);
	dmm32 A1.w, 0xe4660b32;
	dmm32 A1.x, 0xffffff84;
	imm32 R1, 0x2c6c9118;
	imm32 R2, 0x007793ad;
	imm32 R7, 0x526c17d9;
	R1.H = (A1 -= R2.L * R7.L) (M, ISS2);
	checkreg R1, 0x80009118;
	checkreg A1.w, 0xee7d528d;
	checkreg A1.x, 0xffffff84;
	checkreg ASTAT, (0x58608410 | _VS | _V | _AQ | _CC | _V_COPY | _AZ);

	dmm32 ASTAT, (0x2020c210 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY | _AN);
	dmm32 A1.w, 0x8da6c28f;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x0000fff7;
	imm32 R4, 0xf85a0000;
	imm32 R7, 0x7fff0000;
	R7 = (A1 += R4.L * R1.L) (M, ISS2);
	checkreg R7, 0x7fffffff;
	checkreg A1.w, 0x8da6c28f;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2020c210 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY | _AN);

	pass
