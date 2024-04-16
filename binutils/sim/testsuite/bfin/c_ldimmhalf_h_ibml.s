//Original:/proj/frio/dv/testcases/core/c_ldimmhalf_h_ibml/c_ldimmhalf_h_ibml.dsp
// Spec Reference: ldimmhalf h ibml
# mach: bfin

.include "testutils.inc"
	start

	INIT_I_REGS -1;
	INIT_L_REGS -1;
	INIT_B_REGS -1;
	INIT_M_REGS -1;

	I0.H = 0x2000;
	I1.H = 0x2002;
	I2.H = 0x2004;
	I3.H = 0x2006;
	L0.H = 0x2008;
	L1.H = 0x200a;
	L2.H = 0x200c;
	L3.H = 0x200e;

	R0 = I0;
	R1 = I1;
	R2 = I2;
	R3 = I3;
	R4 = L0;
	R5 = L1;
	R6 = L2;
	R7 = L3;
	CHECKREG r0, 0x2000ffff;
	CHECKREG r1, 0x2002ffff;
	CHECKREG r2, 0x2004ffff;
	CHECKREG r3, 0x2006ffff;
	CHECKREG r4, 0x2008ffff;
	CHECKREG r5, 0x200affff;
	CHECKREG r6, 0x200cffff;
	CHECKREG r7, 0x200effff;

	I0.H = 0x0111;
	I1.H = 0x1111;
	I2.H = 0x2222;
	I3.H = 0x3333;
	L0.H = 0x4444;
	L1.H = 0x5555;
	L2.H = 0x6666;
	L3.H = 0x7777;
	R0 = I0;
	R1 = I1;
	R2 = I2;
	R3 = I3;
	R4 = L0;
	R5 = L1;
	R6 = L2;
	R7 = L3;
	CHECKREG r0, 0x0111ffff;
	CHECKREG r1, 0x1111ffff;
	CHECKREG r2, 0x2222ffff;
	CHECKREG r3, 0x3333ffff;
	CHECKREG r4, 0x4444ffff;
	CHECKREG r5, 0x5555ffff;
	CHECKREG r6, 0x6666ffff;
	CHECKREG r7, 0x7777ffff;

	I0.H = 0x8888;
	I1.H = 0x9aaa;
	I2.H = 0xabbb;
	I3.H = 0xbccc;
	L0.H = 0xcddd;
	L1.H = 0xdeee;
	L2.H = 0xefff;
	L3.H = 0xf111;
	R0 = I0;
	R1 = I1;
	R2 = I2;
	R3 = I3;
	R4 = L0;
	R5 = L1;
	R6 = L2;
	R7 = L3;
	CHECKREG r0, 0x8888ffff;
	CHECKREG r1, 0x9aaaffff;
	CHECKREG r2, 0xabbbffff;
	CHECKREG r3, 0xbcccffff;
	CHECKREG r4, 0xcdddffff;
	CHECKREG r5, 0xdeeeffff;
	CHECKREG r6, 0xefffffff;
	CHECKREG r7, 0xf111ffff;

	B0.H = 0x3000;
	B1.H = 0x3002;
	B2.H = 0x3004;
	B3.H = 0x3006;
	M0.H = 0x3008;
	M1.H = 0x300a;
	M2.H = 0x300c;
	M3.H = 0x300e;

	R0 = B0;
	R1 = B1;
	R2 = B2;
	R3 = B3;
	R4 = M0;
	R5 = M1;
	R6 = M2;
	R7 = M3;
	CHECKREG r0, 0x3000ffff;
	CHECKREG r1, 0x3002ffff;
	CHECKREG r2, 0x3004ffff;
	CHECKREG r3, 0x3006ffff;
	CHECKREG r4, 0x3008ffff;
	CHECKREG r5, 0x300Affff;
	CHECKREG r6, 0x300cffff;
	CHECKREG r7, 0x300effff;

	B0.H = 0x0110;
	B1.H = 0x1110;
	B2.H = 0x2220;
	B3.H = 0x3330;
	M0.H = 0x4440;
	M1.H = 0x5550;
	M2.H = 0x6660;
	M3.H = 0x7770;
	R0 = B0;
	R1 = B1;
	R2 = B2;
	R3 = B3;
	R4 = M0;
	R5 = M1;
	R6 = M2;
	R7 = M3;
	CHECKREG r0, 0x0110FFFF;
	CHECKREG r1, 0x1110FFFF;
	CHECKREG r2, 0x2220FFFF;
	CHECKREG r3, 0x3330FFFF;
	CHECKREG r4, 0x4440FFFF;
	CHECKREG r5, 0x5550FFFF;
	CHECKREG r6, 0x6660FFFF;
	CHECKREG r7, 0x7770FFFF;

	B0.H = 0xf880;
	B1.H = 0xfaa0;
	B2.H = 0xfbb0;
	B3.H = 0xfcc0;
	M0.H = 0xfdd0;
	M1.H = 0xfee0;
	M2.H = 0xfff0;
	M3.H = 0xf110;
	R0 = B0;
	R1 = B1;
	R2 = B2;
	R3 = B3;
	R4 = M0;
	R5 = M1;
	R6 = M2;
	R7 = M3;
	CHECKREG r0, 0xf880ffff;
	CHECKREG r1, 0xfaa0ffff;
	CHECKREG r2, 0xfbb0ffff;
	CHECKREG r3, 0xfcc0ffff;
	CHECKREG r4, 0xfdd0ffff;
	CHECKREG r5, 0xfee0ffff;
	CHECKREG r6, 0xfff0ffff;
	CHECKREG r7, 0xf110ffff;

	pass
