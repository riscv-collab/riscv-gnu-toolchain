//Original:/proj/frio/dv/testcases/core/c_ldimmhalf_l_ibml/c_ldimmhalf_l_ibml.dsp
// Spec Reference: ldimmhalf l ibml
# mach: bfin

.include "testutils.inc"
	start

	INIT_I_REGS -1;
	INIT_L_REGS -1;
	INIT_M_REGS -1;
	INIT_B_REGS -1;

	I0.L = 0x2001;
	I1.L = 0x2003;
	I2.L = 0x2005;
	I3.L = 0x2007;
	L0.L = 0x2009;
	L1.L = 0x200b;
	L2.L = 0x200d;
	L3.L = 0x200f;

	R0 = I0;
	R1 = I1;
	R2 = I2;
	R3 = I3;
	R4 = L0;
	R5 = L1;
	R6 = L2;
	R7 = L3;
	CHECKREG r0, 0xffff2001;
	CHECKREG r1, 0xffff2003;
	CHECKREG r2, 0xffff2005;
	CHECKREG r3, 0xffff2007;
	CHECKREG r4, 0xffff2009;
	CHECKREG r5, 0xffff200b;
	CHECKREG r6, 0xffff200d;
	CHECKREG r7, 0xffff200f;

	I0.L = 0x0111;
	I1.L = 0x1111;
	I2.L = 0x2222;
	I3.L = 0x3333;
	L0.L = 0x4444;
	L1.L = 0x5555;
	L2.L = 0x6666;
	L3.L = 0x7777;
	R0 = I0;
	R1 = I1;
	R2 = I2;
	R3 = I3;
	R4 = L0;
	R5 = L1;
	R6 = L2;
	R7 = L3;
	CHECKREG r0, 0xffff0111;
	CHECKREG r1, 0xffff1111;
	CHECKREG r2, 0xffff2222;
	CHECKREG r3, 0xffff3333;
	CHECKREG r4, 0xffff4444;
	CHECKREG r5, 0xffff5555;
	CHECKREG r6, 0xffff6666;
	CHECKREG r7, 0xffff7777;

	I0.L = 0x8888;
	I1.L = 0x9aaa;
	I2.L = 0xabbb;
	I3.L = 0xbccc;
	L0.L = 0xcddd;
	L1.L = 0xdeee;
	L2.L = 0xefff;
	L3.L = 0xf111;
	R0 = I0;
	R1 = I1;
	R2 = I2;
	R3 = I3;
	R4 = L0;
	R5 = L1;
	R6 = L2;
	R7 = L3;
	CHECKREG r0, 0xffff8888;
	CHECKREG r1, 0xffff9aaa;
	CHECKREG r2, 0xffffabbb;
	CHECKREG r3, 0xffffbccc;
	CHECKREG r4, 0xffffcddd;
	CHECKREG r5, 0xffffdeee;
	CHECKREG r6, 0xffffefff;
	CHECKREG r7, 0xfffff111;

	B0.L = 0x3001;
	B1.L = 0x3003;
	B2.L = 0x3005;
	B3.L = 0x3007;
	M0.L = 0x3009;
	M1.L = 0x300b;
	M2.L = 0x300d;
	M3.L = 0x300f;

	R0 = B0;
	R1 = B1;
	R2 = B2;
	R3 = B3;
	R4 = M0;
	R5 = M1;
	R6 = M2;
	R7 = M3;
	CHECKREG r0, 0xffff3001;
	CHECKREG r1, 0xffff3003;
	CHECKREG r2, 0xffff3005;
	CHECKREG r3, 0xffff3007;
	CHECKREG r4, 0xffff3009;
	CHECKREG r5, 0xffff300B;
	CHECKREG r6, 0xffff300d;
	CHECKREG r7, 0xffff300f;

	B0.L = 0x0110;
	B1.L = 0x1110;
	B2.L = 0x2220;
	B3.L = 0x3330;
	M0.L = 0x4440;
	M1.L = 0x5550;
	M2.L = 0x6660;
	M3.L = 0x7770;
	R0 = B0;
	R1 = B1;
	R2 = B2;
	R3 = B3;
	R4 = M0;
	R5 = M1;
	R6 = M2;
	R7 = M3;
	CHECKREG r0, 0xffff0110;
	CHECKREG r1, 0xffff1110;
	CHECKREG r2, 0xffff2220;
	CHECKREG r3, 0xffff3330;
	CHECKREG r4, 0xffff4440;
	CHECKREG r5, 0xffff5550;
	CHECKREG r6, 0xffff6660;
	CHECKREG r7, 0xffff7770;

	B0.L = 0xf880;
	B1.L = 0xfaa0;
	B2.L = 0xfbb0;
	B3.L = 0xfcc0;
	M0.L = 0xfdd0;
	M1.L = 0xfee0;
	M2.L = 0xfff0;
	M3.L = 0xf110;
	R0 = B0;
	R1 = B1;
	R2 = B2;
	R3 = B3;
	R4 = M0;
	R5 = M1;
	R6 = M2;
	R7 = M3;
	CHECKREG r0, 0xfffff880;
	CHECKREG r1, 0xfffffaa0;
	CHECKREG r2, 0xfffffbb0;
	CHECKREG r3, 0xfffffcc0;
	CHECKREG r4, 0xfffffdd0;
	CHECKREG r5, 0xfffffee0;
	CHECKREG r6, 0xfffffff0;
	CHECKREG r7, 0xfffff110;

	pass
