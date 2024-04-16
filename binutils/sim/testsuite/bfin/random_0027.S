# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x2850c890 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY);
	dmm32 A1.w, 0xa605868e;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x56dd0982;
	imm32 R4, 0x50e37862;
	imm32 R5, 0x597fc81a;
	R4.H = (A1 -= R5.L * R1.L) (M, IS);
	checkreg R4, 0x7fff7862;
	checkreg A1.w, 0xa818ff5a;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x2850c890 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x08100a00 | _VS | _AC1 | _AC0 | _CC);
	dmm32 A1.w, 0xeb710132;
	dmm32 A1.x, 0xffffffcf;
	imm32 R4, 0x750d92cc;
	imm32 R7, 0xf9a22cee;
	R4.H = (A1 -= R7.L * R7.H) (M, IS);
	checkreg R4, 0x800092cc;
	checkreg A1.w, 0xbfa11496;
	checkreg A1.x, 0xffffffcf;
	checkreg ASTAT, (0x08100a00 | _VS | _V | _AC1 | _AC0 | _CC | _V_COPY);

	dmm32 ASTAT, (0x44e00410 | _VS | _AV0S | _AQ | _AN);
	dmm32 A1.w, 0x95489ea8;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x360dca41;
	imm32 R4, 0x7fffe848;
	imm32 R7, 0x278abda8;
	R7 = (A1 -= R4.L * R1.L) (M, IS);
	checkreg R7, 0x7fffffff;
	checkreg A1.w, 0xa805d460;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x44e00410 | _VS | _V | _AV0S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x0480c800 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AN | _AZ);
	dmm32 A1.w, 0xcfa4f43b;
	dmm32 A1.x, 0x0000006c;
	imm32 R3, 0x0903dd55;
	imm32 R7, 0x7fffc2b1;
	A1 -= R3.L * R7.L (M, IS);
	checkreg A1.w, 0xea028276;
	checkreg A1.x, 0x0000006c;
	checkreg ASTAT, (0x0480c800 | _VS | _AV1S | _AV0S | _AC0 | _CC | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x3c204410 | _VS | _AV0S | _AN);
	dmm32 A1.w, 0x928b984e;
	dmm32 A1.x, 0xffffffd5;
	imm32 R5, 0x00003ddd;
	imm32 R7, 0x8000ffff;
	A1 += R5.L * R7.L (M, IS);
	checkreg A1.w, 0xd0685a71;
	checkreg A1.x, 0xffffffd5;
	checkreg ASTAT, (0x3c204410 | _VS | _AV0S | _AN);

	dmm32 ASTAT, (0x4840c890 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY | _AN);
	dmm32 A1.w, 0x8837abf1;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x10c90000;
	imm32 R7, 0x7fffe6b8;
	A1 += R7.L * R3.H (M, IS);
	checkreg A1.w, 0x868f5269;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x4840c890 | _VS | _AV1S | _AV0S | _AC0 | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x78604a80 | _VS | _V | _AV0S | _AQ | _CC | _V_COPY);
	dmm32 A1.w, 0xdca875cf;
	dmm32 A1.x, 0x0000002c;
	imm32 R3, 0x4c0892ef;
	imm32 R5, 0x001fea98;
	R5.H = (A1 += R5.L * R3.H) (M, IS);
	checkreg R5, 0x7fffea98;
	checkreg A1.w, 0xd64cea8f;
	checkreg A1.x, 0x0000002c;
	checkreg ASTAT, (0x78604a80 | _VS | _V | _AV0S | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x00a04210 | _VS | _V | _AV1S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xec5ef880;
	dmm32 A1.x, 0xfffffffe;
	imm32 R0, 0x229657d6;
	imm32 R7, 0xedd48000;
	A1 += R0.L * R7.L (M, IS);
	checkreg A1.w, 0x1849f880;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x00a04210 | _VS | _V | _AV1S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x0840ce80 | _VS | _AV1S | _AV0S | _AC1 | _AQ);
	dmm32 A1.w, 0xe4a5a6e1;
	dmm32 A1.x, 0x00000078;
	imm32 R0, 0xf059329d;
	imm32 R7, 0x7fff7512;
	A1 += R7.L * R0.L (M, IS);
	checkreg A1.w, 0xfbcaf6eb;
	checkreg A1.x, 0x00000078;
	checkreg ASTAT, (0x0840ce80 | _VS | _AV1S | _AV0S | _AC1 | _AQ);

	dmm32 ASTAT, (0x60100810 | _VS | _AV0S | _AQ | _AC0_COPY | _AZ);
	dmm32 A1.w, 0xd56a8232;
	dmm32 A1.x, 0x00000033;
	imm32 R0, 0x09b22c69;
	imm32 R7, 0x434f1d64;
	A1 -= R0.L * R7.L (M, IS);
	checkreg A1.w, 0xd051442e;
	checkreg A1.x, 0x00000033;
	checkreg ASTAT, (0x60100810 | _VS | _AV0S | _AQ | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x58e08410 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0x86c9a59e;
	dmm32 A1.x, 0xffffff9a;
	imm32 R1, 0x22573f31;
	imm32 R6, 0x2d0c0155;
	A1 += R1.L * R6.H (M, IS);
	checkreg A1.w, 0x91e838ea;
	checkreg A1.x, 0xffffff9a;
	checkreg ASTAT, (0x58e08410 | _VS | _AV1S | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x64a0c690 | _VS | _AV0S | _AC1 | _AQ | _AC0_COPY | _AN);
	dmm32 A1.w, 0xc5c840aa;
	dmm32 A1.x, 0x00000000;
	imm32 R4, 0xffff7fff;
	imm32 R7, 0x658e833f;
	A1 -= R7.L * R4.H (M, IS);
	checkreg A1.w, 0x4288c3e9;
	checkreg A1.x, 0x00000001;
	checkreg ASTAT, (0x64a0c690 | _VS | _AV0S | _AC1 | _AQ | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x08804610 | _VS | _V | _AV0S | _AC1 | _AQ | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0xf1000000;
	dmm32 A1.x, 0x00000040;
	imm32 R3, 0x0cd4edf1;
	imm32 R6, 0x4dfc08b8;
	R6.H = (A1 += R6.L * R3.H) (M, IS);
	checkreg R6, 0x7fff08b8;
	checkreg A1.w, 0xf16fd860;
	checkreg A1.x, 0x00000040;
	checkreg ASTAT, (0x08804610 | _VS | _V | _AV0S | _AC1 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x7c004690 | _VS | _AV0S | _AC1 | _AQ | _AC0_COPY | _AN);
	dmm32 A1.w, 0xd4deb886;
	dmm32 A1.x, 0x00000001;
	imm32 R1, 0x80008000;
	imm32 R6, 0x22fb6e50;
	imm32 R7, 0x3fcb147f;
	R1.H = (A1 -= R7.L * R6.L) (M, IS);
	checkreg R1, 0x7fff8000;
	checkreg A1.w, 0xcc09bed6;
	checkreg A1.x, 0x00000001;
	checkreg ASTAT, (0x7c004690 | _VS | _V | _AV0S | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x40a00400 | _VS | _V | _AV1S | _AQ | _V_COPY | _AN);
	dmm32 A1.w, 0x9698e35b;
	dmm32 A1.x, 0xfffffffc;
	imm32 R5, 0x8000038c;
	imm32 R6, 0x3152ffff;
	A1 -= R6.L * R5.L (M, IS);
	checkreg A1.w, 0x9698e6e7;
	checkreg A1.x, 0xfffffffc;
	checkreg ASTAT, (0x40a00400 | _VS | _V | _AV1S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x54c00810 | _VS | _V | _AC1 | _CC | _V_COPY | _AN | _AZ);
	dmm32 A1.w, 0x9b02b9c6;
	dmm32 A1.x, 0xffffffd4;
	imm32 R2, 0xff020105;
	imm32 R3, 0xa8ff8000;
	R3.H = (A1 -= R2.L * R3.L) (M, IS);
	checkreg R3, 0x80008000;
	checkreg A1.w, 0x9a8039c6;
	checkreg A1.x, 0xffffffd4;
	checkreg ASTAT, (0x54c00810 | _VS | _V | _AC1 | _CC | _V_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x58808680 | _VS | _V | _AC1 | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0x990456b2;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x2b76c7b2;
	imm32 R3, 0x659803c8;
	imm32 R7, 0x7fffffff;
	R3.H = (A1 += R7.L * R0.L) (M, IS);
	checkreg R3, 0x7fff03c8;
	checkreg A1.w, 0x99038f00;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x58808680 | _VS | _V | _AC1 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x3ce04690 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY);
	dmm32 A1.w, 0x95d1d45a;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x4331b012;
	imm32 R5, 0x7fff8000;
	A1 -= R0.L * R5.H (M, IS);
	checkreg A1.w, 0xbdc8846c;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x3ce04690 | _VS | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY);

	dmm32 ASTAT, (0x30e04410 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _CC);
	dmm32 A1.w, 0xcf49e4c9;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0xe968a740;
	imm32 R3, 0xd7383cd5;
	imm32 R6, 0x5a87c89b;
	R1 = (A1 += R3.L * R6.H) (M, IS);
	checkreg R1, 0x7fffffff;
	checkreg A1.w, 0xe4ccdb1c;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x30e04410 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x2cb04890 | _VS | _AC1 | _AQ | _AC0_COPY);
	dmm32 A1.w, 0x8bdaf471;
	dmm32 A1.x, 0xffffffbd;
	imm32 R3, 0x728d99b1;
	imm32 R7, 0x181d83c2;
	A1 -= R7.L * R3.L (M, IS);
	checkreg A1.w, 0xd671e94f;
	checkreg A1.x, 0xffffffbd;
	checkreg ASTAT, (0x2cb04890 | _VS | _AC1 | _AQ | _AC0_COPY);

	dmm32 ASTAT, (0x20908680 | _VS | _AV0S | _AC1 | _AQ | _CC | _AZ);
	dmm32 A1.w, 0xc1cb8a00;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0xc1e98ea8;
	imm32 R7, 0x0000961f;
	A1 -= R7.L * R1.L (M, IS);
	checkreg A1.w, 0xfccbd3a8;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x20908680 | _VS | _AV0S | _AC1 | _AQ | _CC | _AZ);

	dmm32 ASTAT, (0x64a0cc80 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AZ);
	dmm32 A1.w, 0xfb328cb4;
	dmm32 A1.x, 0xffffff9b;
	imm32 R2, 0x8000ffff;
	imm32 R3, 0x64d21863;
	imm32 R6, 0x3b7618a6;
	R2.H = (A1 += R3.L * R6.H) (M, IS);
	checkreg A1.w, 0x00dc9b56;
	checkreg A1.x, 0xffffff9c;
	checkreg ASTAT, (0x64a0cc80 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AZ);

	dmm32 ASTAT, (0x3c00ca90 | _VS | _V | _AV0S | _AC1 | _V_COPY | _AC0_COPY | _AN);
	dmm32 A1.w, 0xbfb4c632;
	dmm32 A1.x, 0x00000044;
	imm32 R1, 0x7fffffff;
	imm32 R3, 0xf3e9182e;
	imm32 R5, 0x3c94d844;
	R5.H = (A1 += R1.L * R3.H) (M, IS);
	checkreg R5, 0x7fffd844;
	checkreg A1.w, 0xbfb3d249;
	checkreg A1.x, 0x00000044;
	checkreg ASTAT, (0x3c00ca90 | _VS | _V | _AV0S | _AC1 | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x48c0cc10 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);
	dmm32 A1.w, 0x83144651;
	dmm32 A1.x, 0x00000000;
	imm32 R3, 0x04d0ffff;
	imm32 R4, 0x9dc8f8d8;
	imm32 R7, 0x23180d75;
	R3 = (A1 += R4.L * R7.L) (M, IS);
	checkreg R3, 0x7fffffff;
	checkreg A1.w, 0x82b3f909;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x48c0cc10 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);

	pass
