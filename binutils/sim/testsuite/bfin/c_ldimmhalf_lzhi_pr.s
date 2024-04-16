//Original:/proj/frio/dv/testcases/core/c_ldimmhalf_lzhi_pr/c_ldimmhalf_lzhi_pr.dsp
// Spec Reference: ldimmhalf lzhi preg
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS -1;

// test Preg
//lz(p0)=0x0001;
//h(p0) =0x0000;
	P1 = 0x0003 (Z);
	P1.H = 0x0002;
	P2 = 0x0005 (Z);
	P2.H = 0x0004;
	P3 = 0x0007 (Z);
	P3.H = 0x0006;
	P4 = 0x0009 (Z);
	P4.H = 0x0008;
	P5 = 0x000b (Z);
	P5.H = 0x000a;
	FP = 0x000d (Z);
	FP.H = 0x000c;
	SP = 0x000f (Z);
	SP.H = 0x000e;
	CHECKREG p1, 0x00020003;
	CHECKREG p2, 0x00040005;
	CHECKREG p3, 0x00060007;
	CHECKREG p4, 0x00080009;
	CHECKREG p5, 0x000a000b;
	CHECKREG fp, 0x000c000d;
	CHECKREG sp, 0x000e000f;

	P1 = 0x0030 (Z);
	P1.H = 0x0020;
	P2 = 0x0050 (Z);
	P2.H = 0x0040;
	P3 = 0x0070 (Z);
	P3.H = 0x0060;
	P4 = 0x0090 (Z);
	P4.H = 0x0080;
	P5 = 0x00b0 (Z);
	P5.H = 0x00a0;
	FP = 0x00d0 (Z);
	FP.H = 0x00c0;
	SP = 0x00f0 (Z);
	SP.H = 0x00e0;
//CHECKREG p0, 0x00000010;
	CHECKREG p1, 0x00200030;
	CHECKREG p2, 0x00400050;
	CHECKREG p3, 0x00600070;
	CHECKREG p4, 0x00800090;
	CHECKREG p5, 0x00a000b0;
	CHECKREG fp, 0x00c000d0;
	CHECKREG sp, 0x00e000f0;

	P1 = 0x0300 (Z);
	P1.H = 0x0200;
	P2 = 0x0500 (Z);
	P2.H = 0x0400;
	P3 = 0x0700 (Z);
	P3.H = 0x0600;
	P4 = 0x0900 (Z);
	P4.H = 0x0800;
	P5 = 0x0b00 (Z);
	P5.H = 0x0a00;
	FP = 0x0d00 (Z);
	FP.H = 0x0c00;
	SP = 0x0f00 (Z);
	SP.H = 0x0e00;
	CHECKREG p1, 0x02000300;
	CHECKREG p2, 0x04000500;
	CHECKREG p3, 0x06000700;
	CHECKREG p4, 0x08000900;
	CHECKREG p5, 0x0a000b00;
	CHECKREG fp, 0x0c000d00;
	CHECKREG sp, 0x0e000f00;

	P1 = 0x3000 (Z);
	P1.H = 0x2000;
	P2 = 0x5000 (Z);
	P2.H = 0x4000;
	P3 = 0x7000 (Z);
	P3.H = 0x6000;
	P4 = 0x9000 (Z);
	P4.H = 0x8000;
	P5 = 0xb000 (Z);
	P5.H = 0xa000;
	FP = 0xd000 (Z);
	FP.H = 0xc000;
	SP = 0xf000 (Z);
	SP.H = 0xe000;
	CHECKREG p1, 0x20003000;
	CHECKREG p2, 0x40005000;
	CHECKREG p3, 0x60007000;
	CHECKREG p4, 0x80009000;
	CHECKREG p5, 0xa000b000;
	CHECKREG fp, 0xc000d000;
	CHECKREG sp, 0xe000f000;

	pass
