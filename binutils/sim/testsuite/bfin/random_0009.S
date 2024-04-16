# Verify ASTAT bits are set correctly during dsp mac insns
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x4450cc90 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x16ba2677;
	dmm32 A0.x, 0x00000000;
	imm32 R4, 0x80007fff;
	A0 -= R4.H * R4.H (W32);
	checkreg A0.w, 0x96ba2678;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x4450cc90 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x3c30c800 | _VS | _AV0S | _AC1 | _CC);
	dmm32 A0.w, 0xf170d0c7;
	dmm32 A0.x, 0xffffffff;
	imm32 R2, 0x80008000;
	A0 -= R2.H * R2.L (W32);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x3c30c800 | _VS | _AV0S | _AV0 | _AC1 | _CC);

	dmm32 ASTAT, (0x6c200880 | _VS | _AV1S | _AC1 | _AC0 | _CC | _AN);
	dmm32 A0.x, 0x560a1c52;
	dmm32 A0.x, 0xffffffbb;
	imm32 R5, 0x8000ffff;
	A0 = R5.H * R5.H (W32);
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x6c200880 | _VS | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _CC | _AN);

	dmm32 ASTAT, (0x58908a90 | _VS | _AC1 | _AC0 | _AQ);
	dmm32 A0.w, 0x00c5a4e0;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0xffffb33a;
	imm32 R2, 0xffffb33a;
	imm32 R3, 0xb33a4cc6;
	R2 = (A0 -= R0.L * R3.H) (FU);
	checkreg R2, 0x00000000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x58908a90 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x2cc00c90 | _VS | _AC1 | _AC0 | _AQ | _AC0_COPY);
	dmm32 A0.w, 0x00a38000;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x2aa2ffff;
	imm32 R1, 0xff5c711e;
	imm32 R4, 0x2913dc90;
	R0 = (A0 -= R4.L * R1.L) (IU);
	checkreg R0, 0x00000000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x2cc00c90 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x3880c280 | _VS | _AC1 | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R4, 0x139ad315;
	imm32 R6, 0x7fff0000;
	R4.L = (A0 -= R6.H * R6.H) (FU);
	checkreg R4, 0x139a0000;
	checkreg ASTAT, (0x3880c280 | _VS | _V | _AV0S | _AV0 | _AC1 | _V_COPY | _AZ);

	dmm32 ASTAT, (0x48408290 | _VS | _AV1S | _AV0S | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0x6b426a69;
	dmm32 A0.x, 0xffffffba;
	imm32 R0, 0x24038000;
	imm32 R2, 0xf62c7780;
	imm32 R3, 0x5a64f8e8;
	R2.L = (A0 -= R3.L * R0.L) (IH);
	checkreg R2, 0xf62c8000;
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x48408290 | _VS | _V | _AV1S | _AV0S | _AV0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x7c00c210 | _VS | _AC1 | _AN);
	dmm32 A1.w, 0x730173e9;
	dmm32 A1.x, 0xffffffae;
	imm32 R4, 0x8000ffff;
	imm32 R5, 0x738559e8;
	R5.H = (A1 -= R4.L * R5.L) (M, IH);
	checkreg R5, 0x800059e8;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x7c00c210 | _VS | _V | _AV1S | _AV1 | _AC1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x4830c400 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AZ);
	dmm32 A0.w, 0x033a05f0;
	dmm32 A0.x, 0x00000000;
	imm32 R3, 0x5992dd5a;
	imm32 R4, 0x098a889e;
	imm32 R6, 0x8000de08;
	R6.L = (A0 -= R4.L * R3.H) (TFU);
	checkreg R6, 0x80000000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x4830c400 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _CC | _V_COPY | _AZ);

	pass
