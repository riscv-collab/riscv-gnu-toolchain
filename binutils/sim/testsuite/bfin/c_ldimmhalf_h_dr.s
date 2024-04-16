//Original:/testcases/core/c_ldimmhalf_h_dr/c_ldimmhalf_h_dr.dsp
// Spec Reference: ldimmhalf h dreg
# mach: bfin

.include "testutils.inc"
	start



INIT_R_REGS -1;


// test Dreg
R0.H = 0x0000;
R1.H = 0x0002;
R2.H = 0x0004;
R3.H = 0x0006;
R4.H = 0x0008;
R5.H = 0x000a;
R6.H = 0x000c;
R7.H = 0x000e;
CHECKREG r0, 0x0000ffff;
CHECKREG r1, 0x0002ffff;
CHECKREG r2, 0x0004ffff;
CHECKREG r3, 0x0006ffff;
CHECKREG r4, 0x0008ffff;
CHECKREG r5, 0x000affff;
CHECKREG r6, 0x000cffff;
CHECKREG r7, 0x000effff;

R0.H = 0x0000;
R1.H = 0x0020;
R2.H = 0x0040;
R3.H = 0x0060;
R4.H = 0x0080;
R5.H = 0x00a0;
R6.H = 0x00c0;
R7.H = 0x00e0;
CHECKREG r0, 0x0000ffff;
CHECKREG r1, 0x0020ffff;
CHECKREG r2, 0x0040ffff;
CHECKREG r3, 0x0060ffff;
CHECKREG r4, 0x0080ffff;
CHECKREG r5, 0x00a0ffff;
CHECKREG r6, 0x00c0ffff;
CHECKREG r7, 0x00e0ffff;

R0.H = 0x0000;
R1.H = 0x0200;
R2.H = 0x0400;
R3.H = 0x0600;
R4.H = 0x0800;
R5.H = 0x0a00;
R6.H = 0x0c00;
R7.H = 0x0e00;
CHECKREG r0, 0x0000ffff;
CHECKREG r1, 0x0200ffff;
CHECKREG r2, 0x0400ffff;
CHECKREG r3, 0x0600ffff;
CHECKREG r4, 0x0800ffff;
CHECKREG r5, 0x0a00ffff;
CHECKREG r6, 0x0c00ffff;
CHECKREG r7, 0x0e00ffff;

R0.H = 0x0000;
R1.H = 0x2000;
R2.H = 0x4000;
R3.H = 0x6000;
R4.H = 0x8000;
R5.H = 0xa000;
R6.H = 0xc000;
R7.H = 0xe000;
CHECKREG r0, 0x0000ffff;
CHECKREG r1, 0x2000ffff;
CHECKREG r2, 0x4000ffff;
CHECKREG r3, 0x6000ffff;
CHECKREG r4, 0x8000ffff;
CHECKREG r5, 0xa000ffff;
CHECKREG r6, 0xc000ffff;
CHECKREG r7, 0xe000ffff;

pass
