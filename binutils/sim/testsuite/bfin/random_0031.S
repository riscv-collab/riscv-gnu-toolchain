# Check that VS in ASTAT is set with add/sub insns (and not just V)
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x2810c010 | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);
	imm32 R0, 0x27f3a149;
	imm32 R3, 0x3cae7c58;
	imm32 R4, 0x33c97634;
	R3.H = R0.L - R4.H (NS);
	checkreg R3, 0x6d807c58;
	checkreg ASTAT, (0x2810c010 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x64c00680 | _AQ | _AC0_COPY);
	imm32 R1, 0x1b7b025c;
	imm32 R5, 0x1ba46ce6;
	R5.L = R5.L + R1.H (NS);
	checkreg R5, 0x1ba48861;
	checkreg ASTAT, (0x64c00680 | _VS | _V | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x68b04200 | _AV1S | _AV0 | _AC0 | _AQ | _AN);
	imm32 R3, 0x4b91870f;
	imm32 R6, 0x5972bae0;
	imm32 R7, 0x31f7dfb7;
	R7.H = R6.L + R3.L (S);
	checkreg R7, 0x8000dfb7;
	checkreg ASTAT, (0x68b04200 | _VS | _V | _AV1S | _AV0 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x78208e90 | _CC | _AN);
	imm32 R3, 0x40b63bc7;
	imm32 R5, 0x49c89df9;
	R3.H = R5.L - R3.H (NS);
	checkreg R3, 0x5d433bc7;
	checkreg ASTAT, (0x78208e90 | _VS | _V | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x00904680 | _AV1S | _AV1 | _AV0 | _AC1 | _AQ | _AZ);
	imm32 R2, 0x23a2c115;
	imm32 R4, 0x6977581e;
	imm32 R6, 0x41900942;
	R4.L = R2.L - R6.H (NS);
	checkreg R4, 0x69777f85;
	checkreg ASTAT, (0x00904680 | _VS | _V | _AV1S | _AV1 | _AV0 | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x78d08210 | _V | _AV1S | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN | _AZ);
	imm32 R0, 0x4317139e;
	imm32 R1, 0x49ed40d6;
	R0.L = R1.L + R0.H (NS);
	checkreg R0, 0x431783ed;
	checkreg ASTAT, (0x78d08210 | _VS | _V | _AV1S | _AC1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x58d00e10 | _AV1 | _AQ | _CC);
	imm32 R0, 0x09ea77a2;
	imm32 R1, 0x6ccd0b05;
	imm32 R2, 0x761c63af;
	R1.H = R0.L + R2.H (NS);
	checkreg R1, 0xedbe0b05;
	checkreg ASTAT, (0x58d00e10 | _VS | _V | _AV1 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x30c08000 | _AC0 | _AQ | _AC0_COPY);
	imm32 R4, 0x36d243cb;
	imm32 R5, 0xcd127add;
	R4.H = R5.L + R4.L (NS);
	checkreg R4, 0xbea843cb;
	checkreg ASTAT, (0x30c08000 | _VS | _V | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x74108400 | _V | _AV1 | _AC1 | _AC0 | _AC0_COPY);
	imm32 R0, 0x4e1893ea;
	imm32 R1, 0x13cf5cc8;
	imm32 R3, 0x7441949e;
	R1.L = R0.L - R3.H (NS);
	checkreg R1, 0x13cf1fa9;
	checkreg ASTAT, (0x74108400 | _VS | _V | _AV1 | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x7420ce10 | _AV1S | _AV1 | _AV0S | _AC1 | _AQ | _AC0_COPY | _AN | _AZ);
	imm32 R4, 0x532c8fb1;
	imm32 R6, 0x582420d2;
	R6.H = R4.L - R4.H (NS);
	checkreg R6, 0x3c8520d2;
	checkreg ASTAT, (0x7420ce10 | _VS | _V | _AV1S | _AV1 | _AV0S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x74704010 | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY);
	imm32 R3, 0x6f6a7429;
	imm32 R5, 0x2ea5c47e;
	R5.H = R5.L - R3.H (NS);
	checkreg R5, 0x5514c47e;
	checkreg ASTAT, (0x74704010 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x0ce08490 | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AZ);
	imm32 R1, 0xfd18a0b0;
	imm32 R4, 0x259e2151;
	R4.L = R1.L - R4.H (NS);
	checkreg R4, 0x259e7b12;
	checkreg ASTAT, (0x0ce08490 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x54b08810 | _V | _AV1S | _AV0S | _AC0_COPY | _AN);
	imm32 R3, 0x7a763675;
	imm32 R6, 0x23c4a335;
	R3.L = R6.L + R6.L (NS);
	checkreg R3, 0x7a76466a;
	checkreg ASTAT, (0x54b08810 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x70f0c080 | _AV1S | _AV0S | _AC0);
	imm32 R4, 0x55fab7e4;
	imm32 R5, 0x7dbd9b06;
	R5.H = R5.L - R4.H (S);
	checkreg R5, 0x80009b06;
	checkreg ASTAT, (0x70f0c080 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x5ce04680 | _AV0 | _AC0 | _V_COPY | _AC0_COPY | _AN);
	imm32 R0, 0x19cacbdb;
	imm32 R2, 0x151cb293;
	imm32 R4, 0x571c351a;
	R0.H = R4.L - R2.L (S);
	checkreg R0, 0x7fffcbdb;
	checkreg ASTAT, (0x5ce04680 | _VS | _V | _AV0 | _V_COPY);

	dmm32 ASTAT, (0x0c604a00 | _AV1S | _AV0S | _V_COPY | _AC0_COPY | _AZ);
	imm32 R3, 0x5432c45d;
	imm32 R6, 0x62519952;
	R3.L = R6.L + R6.L (S);
	checkreg R3, 0x54328000;
	checkreg ASTAT, (0x0c604a00 | _VS | _V | _AV1S | _AV0S | _AC0 | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x58708c90 | _AV0 | _AC1 | _AQ | _CC | _AC0_COPY | _AN | _AZ);
	imm32 R0, 0x1f3f3c0e;
	imm32 R4, 0x5fae58d2;
	R0.H = R0.L + R4.L (NS);
	checkreg R0, 0x94e03c0e;
	checkreg ASTAT, (0x58708c90 | _VS | _V | _AV0 | _AC1 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x34b00a00 | _V | _AV1S | _AC1 | _CC | _V_COPY | _AZ);
	imm32 R3, 0x6ea226dc;
	imm32 R4, 0x045c6d64;
	imm32 R7, 0x7e599a25;
	R7.L = R3.L + R4.L (NS);
	checkreg R7, 0x7e599440;
	checkreg ASTAT, (0x34b00a00 | _VS | _V | _AV1S | _AC1 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x40a0c010 | _AV1S | _AC0);
	imm32 R2, 0x641501ef;
	imm32 R7, 0x3acb49aa;
	R2.H = R7.L + R7.H (NS);
	checkreg R2, 0x847501ef;
	checkreg ASTAT, (0x40a0c010 | _VS | _V | _AV1S | _V_COPY | _AN);

	dmm32 ASTAT, (0x78f04090 | _AV1S | _AC1 | _AQ | _CC | _AZ);
	imm32 R2, 0x65681fdf;
	imm32 R3, 0x5fffe0d3;
	imm32 R5, 0x37df99cd;
	R2.H = R5.L - R3.H (NS);
	checkreg R2, 0x39ce1fdf;
	checkreg ASTAT, (0x78f04090 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x0cc04e10 | _AV1S | _AQ | _CC);
	imm32 R3, 0x571977df;
	imm32 R4, 0x029671d0;
	R3.L = R4.L + R3.H (NS);
	checkreg R3, 0x5719c8e9;
	checkreg ASTAT, (0x0cc04e10 | _VS | _V | _AV1S | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x00104880 | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	imm32 R0, 0x4c98aa07;
	imm32 R4, 0x5e9da59f;
	R4.H = R0.L + R0.L (S);
	checkreg R4, 0x8000a59f;
	checkreg ASTAT, (0x00104880 | _VS | _V | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x08008c00 | _AV1S | _AV0S | _AV0 | _CC | _AC0_COPY);
	imm32 R4, 0x58ee2400;
	imm32 R6, 0x2e97af3e;
	R4.L = R6.L + R6.L (NS);
	checkreg R4, 0x58ee5e7c;
	checkreg ASTAT, (0x08008c00 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x2ce0c290 | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	imm32 R2, 0x2d467e64;
	imm32 R6, 0x31aeb601;
	imm32 R7, 0x1523a746;
	R7.L = R2.L - R6.L (S);
	checkreg R7, 0x15237fff;
	checkreg ASTAT, (0x2ce0c290 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);

	pass
