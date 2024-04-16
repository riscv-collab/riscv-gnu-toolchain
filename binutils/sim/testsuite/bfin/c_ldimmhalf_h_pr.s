//Original:/proj/frio/dv/testcases/core/c_ldimmhalf_h_pr/c_ldimmhalf_h_pr.dsp
// Spec Reference: ldimmhalf h preg
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS -1;
	INIT_P_REGS -1;
	imm32 sp, 0xffffffff;
	imm32 fp, 0xffffffff;

// test Preg
	P1.H = 0x0002;
	P2.H = 0x0004;
	P3.H = 0x0006;
	P4.H = 0x0008;
	P5.H = 0x000a;
	FP.H = 0x000c;
	SP.H = 0x000e;
	CHECKREG p1, 0x0002ffff;
	CHECKREG p2, 0x0004ffff;
	CHECKREG p3, 0x0006ffff;
	CHECKREG p4, 0x0008ffff;
	CHECKREG p5, 0x000affff;
	CHECKREG fp, 0x000cffff;
	CHECKREG sp, 0x000effff;

	P1.H = 0x0020;
	P2.H = 0x0040;
	P3.H = 0x0060;
	P4.H = 0x0080;
	P5.H = 0x00a0;
	FP.H = 0x00c0;
	SP.H = 0x00e0;
	CHECKREG p1, 0x0020ffff;
	CHECKREG p2, 0x0040ffff;
	CHECKREG p3, 0x0060ffff;
	CHECKREG p4, 0x0080ffff;
	CHECKREG p5, 0x00a0ffff;
	CHECKREG fp, 0x00c0ffff;
	CHECKREG sp, 0x00e0ffff;

	P1.H = 0x0200;
	P2.H = 0x0400;
	P3.H = 0x0600;
	P4.H = 0x0800;
	P5.H = 0x0a00;
	FP.H = 0x0c00;
	SP.H = 0x0e00;
	CHECKREG p1, 0x0200ffff;
	CHECKREG p2, 0x0400ffff;
	CHECKREG p3, 0x0600ffff;
	CHECKREG p4, 0x0800ffff;
	CHECKREG p5, 0x0a00ffff;
	CHECKREG fp, 0x0c00ffff;
	CHECKREG sp, 0x0e00ffff;

	P1.H = 0x2000;
	P2.H = 0x4000;
	P3.H = 0x6000;
	P4.H = 0x8000;
	P5.H = 0xa000;
	FP.H = 0xc000;
	SP.H = 0xe000;
	CHECKREG p1, 0x2000ffff;
	CHECKREG p2, 0x4000ffff;
	CHECKREG p3, 0x6000ffff;
	CHECKREG p4, 0x8000ffff;
	CHECKREG p5, 0xa000ffff;
	CHECKREG fp, 0xc000ffff;
	CHECKREG sp, 0xe000ffff;

	pass
