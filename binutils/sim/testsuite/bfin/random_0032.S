# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x74308400 | _VS | _AV1S | _AV0S | _CC | _AN);
	dmm32 A0.w, 0x5d4cf98c;
	dmm32 A0.x, 0xffffffff;
	imm32 R0, 0xba16ffff;
	imm32 R4, 0x8000109d;
	imm32 R6, 0x8000b212;
	R6.L = (A0 -= R4.L * R0.L) (IH);
	checkreg R6, 0x80008000;
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x74308400 | _VS | _V | _AV1S | _AV0S | _AV0 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x34e0ce80 | _VS | _V | _AV1S | _V_COPY | _AN);
	dmm32 A0.w, 0x64bb88af;
	dmm32 A0.x, 0xffffffff;
	imm32 R5, 0x00008000;
	imm32 R7, 0x0001ad69;
	R5.L = (A0 += R7.H * R7.L) (IH);
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x34e0ce80 | _VS | _V | _AV1S | _AV0S | _AV0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x4c204c10 | _VS | _V | _AV0S | _AQ | _V_COPY | _AN);
	dmm32 A1.w, 0x75642aaf;
	dmm32 A1.x, 0xffffffff;
	imm32 R2, 0x133dffff;
	imm32 R4, 0xc00006aa;
	imm32 R7, 0x7fffffff;
	R4.H = (A1 -= R2.L * R7.H) (IH);
	checkreg R4, 0x800006aa;
	checkreg A1.w, 0x80000000;
	checkreg A1.x, 0xffffffff;
	checkreg ASTAT, (0x4c204c10 | _VS | _V | _AV1S | _AV1 | _AV0S | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x48600400 | _VS | _V | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AN);
	dmm32 A0.w, 0x534a596c;
	dmm32 A0.x, 0xffffffff;
	imm32 R1, 0x7fff86a7;
	imm32 R5, 0x1163d244;
	R1.L = (A0 -= R5.L * R1.L) (IH);
	checkreg R1, 0x7fff8000;
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x48600400 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x38008c90 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AN);
	dmm32 A1.w, 0x80000000;
	dmm32 A1.x, 0xffffffff;
	imm32 R0, 0x7fffffff;
	imm32 R1, 0xdee9214c;
	imm32 R4, 0x79f3c80a;
	R1.H = (A1 += R0.L * R4.H) (M, IH);
	checkreg R1, 0x8000214c;
	checkreg ASTAT, (0x38008c90 | _VS | _AV1S | _AV1 | _AC1 | _CC | _AN);

	dmm32 ASTAT, (0x4cb00a00 | _VS | _AV1S | _AV0S | _AC1 | _AN);
	dmm32 A0.w, 0x804e7e2f;
	dmm32 A0.x, 0xffffffff;
	imm32 R1, 0x3fccdf09;
	imm32 R2, 0x09e71015;
	imm32 R6, 0x761ac984;
	R2.L = (A0 += R6.L * R1.H) (IH);
	checkreg R2, 0x09e78000;
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x4cb00a00 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x08904c00 | _VS | _AV0S | _AQ | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R1, 0x80000000;
	imm32 R2, 0x0001de54;
	imm32 R5, 0x80000000;
	R1.L = (A0 -= R5.H * R2.H) (TFU);
	checkreg ASTAT, (0x08904c00 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R1, 0x80000000;
	checkreg R2, 0x0001de54;
	checkreg R5, 0x80000000;

	dmm32 ASTAT, (0x00d04810 | _VS | _AV0S | _CC | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R4, 0x00020000;
	imm32 R5, 0x35a26677;
	R4.L = (A0 -= R5.H * R4.H) (TFU);
	checkreg ASTAT, (0x00d04810 | _VS | _V | _AV0S | _AV0 | _CC | _V_COPY | _AC0_COPY | _AZ);
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg R4, 0x00020000;
	checkreg R5, 0x35a26677;

	dmm32 ASTAT, (0x08100a80 | _VS | _AV0S | _AQ | _CC);
	dmm32 A0.w, 0x00000000;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x000300cc;
	imm32 R4, 0x00029150;
	imm32 R7, 0x00ff00ff;
	R4.L = (A0 -= R0.L * R7.L) (IU);
	checkreg R4, 0x00020000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x08100a80 | _VS | _V | _AV0S | _AV0 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x6c20c400 | _VS | _AV1S | _AV0S | _CC);
	dmm32 A0.w, 0x860c9ac9;
	dmm32 A0.x, 0xffffffff;
	imm32 R2, 0x860c9a1b;
	R2.L = (A0 -= R2.H * R2.L) (IH);
	checkreg R2, 0x860c8000;
	checkreg A0.w, 0x80000000;
	checkreg A0.x, 0xffffffff;
	checkreg ASTAT, (0x6c20c400 | _VS | _V | _AV1S | _AV0S | _AV0 | _CC | _V_COPY);

	dmm32 ASTAT, (0x20f00c10 | _VS | _AV0S | _AQ);
	dmm32 A0.w, 0x0000de90;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x00000003;
	imm32 R1, 0xfffd8000;
	imm32 R5, 0x4a31921c;
	R1.L = (A0 -= R5.L * R0.L) (FU);
	checkreg R1, 0xfffd0000;
	checkreg A0.w, 0x00000000;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x20f00c10 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x38700690 | _VS | _V | _AV1S | _AC1 | _AQ | _V_COPY);
	dmm32 A1.w, 0x00000000;
	dmm32 A1.x, 0x00000000;
	imm32 R0, 0x58863d39;
	imm32 R1, 0x45377355;
	imm32 R6, 0x00030000;
	R1.H = (A1 -= R0.L * R6.H) (TFU);
	checkreg R1, 0x00007355;
	checkreg ASTAT, (0x38700690 | _VS | _V | _AV1S | _AV1 | _AC1 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x48704880 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY | _AN);
	dmm32 A0.w, 0x7fffd68a;
	dmm32 A0.x, 0xffffffff;
	imm32 R7, 0x06d88000;
	R7.L = A0 (IH);
	checkreg A0.w, 0x7fffd68a;
	checkreg A0.x, 0xffffffff;
	checkreg R7, 0x06d88000;
	checkreg ASTAT, (0x48704880 | _VS | _AV0S | _AC1 | _AC0 | _AQ | _AC0_COPY | _AN);

	pass
