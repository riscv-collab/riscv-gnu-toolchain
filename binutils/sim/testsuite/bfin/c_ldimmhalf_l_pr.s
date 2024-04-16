//Original:/proj/frio/dv/testcases/core/c_ldimmhalf_l_pr/c_ldimmhalf_l_pr.dsp
// Spec Reference: ldimmhalf l preg
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS -1;
	INIT_P_REGS -1;

	imm32 sp, 0xffffffff;
	imm32 fp, 0xffffffff;

// test Preg
	P1.L = 0x0003;
	P2.L = 0x0005;
	P3.L = 0x0007;
	P4.L = 0x0009;
	P5.L = 0x000b;
	FP.L = 0x000d;
	SP.L = 0x000f;
	CHECKREG p1, 0xffff0003;
	CHECKREG p2, 0xffff0005;
	CHECKREG p3, 0xffff0007;
	CHECKREG p4, 0xffff0009;
	CHECKREG p5, 0xffff000b;
	CHECKREG fp, 0xffff000d;
	CHECKREG sp, 0xffff000f;

	P1.L = 0x0030;
	P2.L = 0x0050;
	P3.L = 0x0070;
	P4.L = 0x0090;
	P5.L = 0x00b0;
	FP.L = 0x00d0;
	SP.L = 0x00f0;
//CHECKREG p0, 0x00000010;
	CHECKREG p1, 0xffff0030;
	CHECKREG p2, 0xffff0050;
	CHECKREG p3, 0xffff0070;
	CHECKREG p4, 0xffff0090;
	CHECKREG p5, 0xffff00b0;
	CHECKREG fp, 0xffff00d0;
	CHECKREG sp, 0xffff00f0;

	P1.L = 0x0300;
	P2.L = 0x0500;
	P3.L = 0x0700;
	P4.L = 0x0900;
	P5.L = 0x0b00;
	FP.L = 0x0d00;
	SP.L = 0x0f00;
	CHECKREG p1, 0xffff0300;
	CHECKREG p2, 0xffff0500;
	CHECKREG p3, 0xffff0700;
	CHECKREG p4, 0xffff0900;
	CHECKREG p5, 0xffff0b00;
	CHECKREG fp, 0xffff0d00;
	CHECKREG sp, 0xffff0f00;

	P1.L = 0x3000;
	P2.L = 0x5000;
	P3.L = 0x7000;
	P4.L = 0x9000;
	P5.L = 0xb000;
	FP.L = 0xd000;
	SP.L = 0xf000;
	CHECKREG p1, 0xffff3000;
	CHECKREG p2, 0xffff5000;
	CHECKREG p3, 0xffff7000;
	CHECKREG p4, 0xffff9000;
	CHECKREG p5, 0xffffb000;
	CHECKREG fp, 0xffffd000;
	CHECKREG sp, 0xfffff000;

	pass
