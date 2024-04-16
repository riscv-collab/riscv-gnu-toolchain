# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x44004010 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY);
	dmm32 A1.w, 0x851fa4fc;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x00000000;
	imm32 R2, 0x80000000;
	imm32 R5, 0x139d77b4;
	R5.H = (A1 += R2.L * R0.L) (M, S2RND);
	checkreg R5, 0x7fff77b4;
	checkreg A1.w, 0x851fa4fc;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x44004010 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x48000600 | _VS | _V | _AV1S | _CC | _V_COPY);
	dmm32 A1.w, 0xc5ee7420;
	dmm32 A1.x, 0x00000000;
	imm32 R1, 0x45f17fff;
	imm32 R2, 0x00000000;
	imm32 R4, 0xffffffff;
	R1 = (A1 -= R2.L * R4.H) (M, S2RND);
	checkreg R1, 0x7fffffff;
	checkreg A1.w, 0xc5ee7420;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x48000600 | _VS | _V | _AV1S | _CC | _V_COPY);

	dmm32 ASTAT, (0x48500a10 | _VS | _V | _AV1S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AZ);
	dmm32 A1.w, 0x965cddab;
	dmm32 A1.x, 0x00000063;
	imm32 R1, 0x1d4cc3e7;
	imm32 R3, 0xe7ce9d8e;
	imm32 R6, 0x3cc80b2f;
	R6.H = (A1 -= R3.L * R1.L) (M, S2RND);
	checkreg R6, 0x7fff0b2f;
	checkreg A1.w, 0xe1b28889;
	checkreg A1.x, 0x00000063;
	checkreg ASTAT, (0x48500a10 | _VS | _V | _AV1S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x44308410 | _VS | _AV0S | _CC | _AN);
	dmm32 A1.w, 0x92315df7;
	dmm32 A1.x, 0x0000007e;
	imm32 R1, 0x9e4b24e0;
	imm32 R4, 0xe3da8000;
	imm32 R7, 0x00ba086c;
	R1.H = (A1 -= R7.L * R4.H) (M, S2RND);
	checkreg R1, 0x7fff24e0;
	checkreg A1.w, 0x8ab26dff;
	checkreg A1.x, 0x0000007e;
	checkreg ASTAT, (0x44308410 | _VS | _V | _AV0S | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x10a00090 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0x8ed084bf;
	dmm32 A1.x, 0xffffffbe;
	imm32 R0, 0x8000ffff;
	imm32 R3, 0xbb4e34ef;
	imm32 R5, 0x7af8492d;
	R5 = (A1 += R3.L * R0.L) (M, S2RND);
	checkreg R5, 0x80000000;
	checkreg A1.w, 0xc3bf4fd0;
	checkreg A1.x, 0xffffffbe;
	checkreg ASTAT, (0x10a00090 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x10f04e10 | _VS | _V | _AV1S | _AV0S | _AC1 | _AN | _AZ);
	dmm32 A1.w, 0x81becdd8;
	dmm32 A1.x, 0x00000058;
	imm32 R2, 0x14946201;
	imm32 R4, 0x1a162edd;
	R2.H = (A1 -= R2.L * R4.L) (M, S2RND);
	checkreg R2, 0x7fff6201;
	checkreg A1.w, 0x6fce04fb;
	checkreg A1.x, 0x00000058;
	checkreg ASTAT, (0x10f04e10 | _VS | _V | _AV1S | _AV0S | _AC1 | _V_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x20f04c80 | _VS | _AV0S | _AN);
	dmm32 A1.w, 0xe9cc0041;
	dmm32 A1.x, 0x00000079;
	imm32 R1, 0x0f62a5a2;
	imm32 R3, 0x4e8e9bdd;
	imm32 R7, 0x6630d991;
	R1 = (A1 -= R3.L * R7.H) (M, S2RND);
	checkreg R1, 0x7fffffff;
	checkreg A1.w, 0x11c4b8d1;
	checkreg A1.x, 0x0000007a;
	checkreg ASTAT, (0x20f04c80 | _VS | _V | _AV0S | _V_COPY | _AN);

	dmm32 ASTAT, (0x20104e00 | _VS | _AC1 | _AC0 | _AQ | _AN);
	dmm32 A1.w, 0xadeb5c67;
	dmm32 A1.x, 0xffffffa6;
	imm32 R1, 0x07911840;
	imm32 R7, 0x01070000;
	R7 = (A1 += R1.L * R7.H) (M, S2RND);
	checkreg R7, 0x80000000;
	checkreg A1.w, 0xae044627;
	checkreg A1.x, 0xffffffa6;
	checkreg ASTAT, (0x20104e00 | _VS | _V | _AC1 | _AC0 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x08e04010 | _VS | _AV0S);
	dmm32 A1.w, 0xff80f384;
	dmm32 A1.x, 0x00000003;
	imm32 R1, 0x00000000;
	imm32 R2, 0x8000387c;
	imm32 R3, 0x1e547fff;
	R2.H = (A1 -= R1.L * R3.L) (M, S2RND);
	checkreg R2, 0x7fff387c;
	checkreg A1.w, 0xff80f384;
	checkreg A1.x, 0x00000003;
	checkreg ASTAT, (0x08e04010 | _VS | _V | _AV0S | _V_COPY);

	dmm32 ASTAT, (0x0cf08280 | _VS | _AV1S | _AC1 | _CC | _AN);
	dmm32 A1.w, 0x80000000;
	dmm32 A1.x, 0xffffff80;
	imm32 R2, 0xecc35cac;
	imm32 R4, 0x00007fff;
	imm32 R7, 0x80000000;
	R7 = (A1 -= R4.L * R2.L) (M, S2RND);
	checkreg R7, 0x80000000;
	checkreg A1.w, 0x51aa5cac;
	checkreg A1.x, 0xffffff80;
	checkreg ASTAT, (0x0cf08280 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x40c08090 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0xfcbe6525;
	dmm32 A1.x, 0x00000039;
	imm32 R0, 0x0003f3c0;
	imm32 R2, 0xfffffffc;
	imm32 R6, 0xffff0000;
	R0.H = (A1 -= R2.L * R6.H) (M, S2RND);
	checkreg R0, 0x7ffff3c0;
	checkreg A1.w, 0xfcc26521;
	checkreg A1.x, 0x00000039;
	checkreg ASTAT, (0x40c08090 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x00704c10 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY);
	dmm32 A1.w, 0xdfbb3c19;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x50407788;
	imm32 R4, 0x50407788;
	imm32 R6, 0x0d3f0c0a;
	R6.H = (A1 -= R4.L * R0.L) (M, S2RND);
	checkreg R6, 0x7fff0c0a;
	checkreg A1.w, 0xa7eb83d9;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x00704c10 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x3c50c610 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0xbc7ca70b;
	dmm32 A1.x, 0xffffff80;
	imm32 R1, 0x76b3a772;
	imm32 R2, 0x5cc87864;
	imm32 R5, 0x33169c34;
	R1 = (A1 += R2.L * R5.H) (M, S2RND);
	checkreg R1, 0x80000000;
	checkreg A1.w, 0xd482eba3;
	checkreg A1.x, 0xffffff80;
	checkreg ASTAT, (0x3c50c610 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x50008480 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY);
	dmm32 A1.w, 0xd843bd0f;
	dmm32 A1.x, 0x00000027;
	imm32 R0, 0xc5d36b7c;
	imm32 R7, 0x7fff8000;
	R0.H = (A1 += R0.L * R7.L) (M, S2RND);
	checkreg R0, 0x7fff6b7c;
	checkreg A1.w, 0x0e01bd0f;
	checkreg A1.x, 0x00000028;
	checkreg ASTAT, (0x50008480 | _VS | _V | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x50208610 | _VS | _V | _AV1S | _AV0S | _V_COPY | _AN);
	dmm32 A1.w, 0xcf30f0be;
	dmm32 A1.x, 0xffffffad;
	imm32 R0, 0x6d8f3470;
	imm32 R4, 0x4174b386;
	imm32 R6, 0x0793b3dd;
	R0.H = (A1 -= R4.L * R6.H) (M, S2RND);
	checkreg R0, 0x80003470;
	checkreg A1.w, 0xd17430cc;
	checkreg A1.x, 0xffffffad;
	checkreg ASTAT, (0x50208610 | _VS | _V | _AV1S | _AV0S | _V_COPY | _AN);

	dmm32 ASTAT, (0x60700c10 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY);
	dmm32 A1.w, 0xc867b111;
	dmm32 A1.x, 0xffffff90;
	imm32 R4, 0x580f445e;
	imm32 R5, 0x1fb2e64b;
	imm32 R6, 0xb6bc814b;
	R6.H = (A1 += R5.L * R4.L) (M, S2RND);
	checkreg R6, 0x8000814b;
	checkreg A1.w, 0xc18a2c9b;
	checkreg A1.x, 0xffffff90;
	checkreg ASTAT, (0x60700c10 | _VS | _V | _AV1S | _AV0S | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x4070c080 | _AV0S | _CC);
	dmm32 A1.w, 0xe1239b9f;
	dmm32 A1.x, 0xffffffcd;
	imm32 R4, 0xe4d2beb4;
	imm32 R5, 0x1c919600;
	imm32 R6, 0x18356124;
	R5.H = (A1 -= R4.L * R6.L) (M, S2RND);
	checkreg R5, 0x80009600;
	checkreg A1.w, 0xf9ea964f;
	checkreg A1.x, 0xffffffcd;
	checkreg ASTAT, (0x4070c080 | _VS | _V | _AV0S | _CC | _V_COPY);

	dmm32 ASTAT, (0x50608210 | _VS | _AV1S | _AV0S | _AV0 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A1.w, 0xe8c00d5a;
	dmm32 A1.x, 0xffffffbe;
	imm32 R1, 0x2baf99f2;
	imm32 R4, 0x03e69887;
	imm32 R7, 0x07f45a0f;
	R1 = (A1 -= R7.L * R4.H) (M, S2RND);
	checkreg R1, 0x80000000;
	checkreg A1.w, 0xe760f6e0;
	checkreg A1.x, 0xffffffbe;
	checkreg ASTAT, (0x50608210 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	pass
