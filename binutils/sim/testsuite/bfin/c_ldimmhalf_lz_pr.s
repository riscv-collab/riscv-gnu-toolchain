//Original:/proj/frio/dv/testcases/core/c_ldimmhalf_lz_pr/c_ldimmhalf_lz_pr.dsp
// Spec Reference: ldimmhalf lz preg
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS -1;

// test Preg
	P1 = 0x0003 (Z);
	P2 = 0x0005 (Z);
	P3 = 0x0007 (Z);
	P4 = 0x0009 (Z);
	P5 = 0x000b (Z);
	FP = 0x000d (Z);
	SP = 0x000f (Z);
	CHECKREG p1, 0x00000003;
	CHECKREG p2, 0x00000005;
	CHECKREG p3, 0x00000007;
	CHECKREG p4, 0x00000009;
	CHECKREG p5, 0x0000000b;
	CHECKREG fp, 0x0000000d;
	CHECKREG sp, 0x0000000f;

	P1 = 0x0030 (Z);
	P2 = 0x0050 (Z);
	P3 = 0x0070 (Z);
	P4 = 0x0090 (Z);
	P5 = 0x00b0 (Z);
	FP = 0x00d0 (Z);
	SP = 0x00f0 (Z);
//CHECKREG p0, 0x00000010;
	CHECKREG p1, 0x00000030;
	CHECKREG p2, 0x00000050;
	CHECKREG p3, 0x00000070;
	CHECKREG p4, 0x00000090;
	CHECKREG p5, 0x000000b0;
	CHECKREG fp, 0x000000d0;
	CHECKREG sp, 0x000000f0;

	P1 = 0x0300 (Z);
	P2 = 0x0500 (Z);
	P3 = 0x0700 (Z);
	P4 = 0x0900 (Z);
	P5 = 0x0b00 (Z);
	FP = 0x0d00 (Z);
	SP = 0x0f00 (Z);
	CHECKREG p1, 0x00000300;
	CHECKREG p2, 0x00000500;
	CHECKREG p3, 0x00000700;
	CHECKREG p4, 0x00000900;
	CHECKREG p5, 0x00000b00;
	CHECKREG fp, 0x00000d00;
	CHECKREG sp, 0x00000f00;

	P1 = 0x3000 (Z);
	P2 = 0x5000 (Z);
	P3 = 0x7000 (Z);
	P4 = 0x9000 (Z);
	P5 = 0xb000 (Z);
	FP = 0xd000 (Z);
	SP = 0xf000 (Z);
	CHECKREG p1, 0x00003000;
	CHECKREG p2, 0x00005000;
	CHECKREG p3, 0x00007000;
	CHECKREG p4, 0x00009000;
	CHECKREG p5, 0x0000b000;
	CHECKREG fp, 0x0000d000;
	CHECKREG sp, 0x0000f000;

	pass
