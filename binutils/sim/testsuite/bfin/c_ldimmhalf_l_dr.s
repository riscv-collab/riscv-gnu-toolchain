//Original:/testcases/core/c_ldimmhalf_l_dr/c_ldimmhalf_l_dr.dsp
// Spec Reference: ldimmhalf l dreg
# mach: bfin

.include "testutils.inc"
	start



INIT_R_REGS -1;


// test Dreg
R0.L = 0x0001;
R1.L = 0x0003;
R2.L = 0x0005;
R3.L = 0x0007;
R4.L = 0x0009;
R5.L = 0x000b;
R6.L = 0x000d;
R7.L = 0x000f;
CHECKREG r0, 0xffff0001;
CHECKREG r1, 0xffff0003;
CHECKREG r2, 0xffff0005;
CHECKREG r3, 0xffff0007;
CHECKREG r4, 0xffff0009;
CHECKREG r5, 0xffff000b;
CHECKREG r6, 0xffff000d;
CHECKREG r7, 0xffff000f;

R0.L = 0x0010;
R1.L = 0x0030;
R2.L = 0x0050;
R3.L = 0x0070;
R4.L = 0x0090;
R5.L = 0x00b0;
R6.L = 0x00d0;
R7.L = 0x00f0;
CHECKREG r0, 0xffff0010;
CHECKREG r1, 0xffff0030;
CHECKREG r2, 0xffff0050;
CHECKREG r3, 0xffff0070;
CHECKREG r4, 0xffff0090;
CHECKREG r5, 0xffff00b0;
CHECKREG r6, 0xffff00d0;
CHECKREG r7, 0xffff00f0;

R0.L = 0x0100;
R1.L = 0x0300;
R2.L = 0x0500;
R3.L = 0x0700;
R4.L = 0x0900;
R5.L = 0x0b00;
R6.L = 0x0d00;
R7.L = 0x0f00;
CHECKREG r0, 0xffff0100;
CHECKREG r1, 0xffff0300;
CHECKREG r2, 0xffff0500;
CHECKREG r3, 0xffff0700;
CHECKREG r4, 0xffff0900;
CHECKREG r5, 0xffff0b00;
CHECKREG r6, 0xffff0d00;
CHECKREG r7, 0xffff0f00;

R0.L = 0x1000;
R1.L = 0x3000;
R2.L = 0x5000;
R3.L = 0x7000;
R4.L = 0x9000;
R5.L = 0xb000;
R6.L = 0xd000;
R7.L = 0xf000;
CHECKREG r0, 0xffff1000;
CHECKREG r1, 0xffff3000;
CHECKREG r2, 0xffff5000;
CHECKREG r3, 0xffff7000;
CHECKREG r4, 0xffff9000;
CHECKREG r5, 0xffffb000;
CHECKREG r6, 0xffffd000;
CHECKREG r7, 0xfffff000;

pass
