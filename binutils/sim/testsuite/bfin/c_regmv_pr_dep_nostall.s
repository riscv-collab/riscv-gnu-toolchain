//Original:/proj/frio/dv/testcases/core/c_regmv_pr_dep_nostall/c_regmv_pr_dep_nostall.dsp
// Spec Reference: regmv pr-dep no stall
# mach: bfin

.include "testutils.inc"
	start

//imm32 p0, 0x00001111;
	imm32 p1, 0x32213330;
	imm32 p2, 0x34415550;
	imm32 p3, 0x36617770;
	imm32 p4, 0x38819990;
	imm32 p5, 0x3aa1bbb0;
	imm32 fp, 0x3cc1ddd0;
	imm32 sp, 0x3ee1fff0;
// P-reg to P-reg to R-reg: no stall
	P4 = P1;
	R1 = P4;
	SP = P5;
	R2 = SP;
	P1 = FP;
	R3 = P1;
	CHECKREG r1, 0x32213330;
	CHECKREG r2, 0x3AA1BBB0;
	CHECKREG r3, 0x3CC1DDD0;

//imm32 p0, 0x00001111;
	imm32 p1, 0x22213332;
	imm32 p2, 0x44415552;
	imm32 p3, 0x66617772;
	imm32 p4, 0x88819992;
	imm32 p5, 0xaaa1bbb2;
	imm32 fp, 0xccc1ddd2;
	imm32 sp, 0xeee1fff2;

// P-reg to P-reg to I reg: no stall
	P1 = P2;
	I0 = P1;
	P3 = P2;
	I1 = P3;
	P5 = P4;
	I2 = P5;
	FP = SP;
	I3 = FP;

	R4 = I3;
	R5 = I2;
	R6 = I1;
	R7 = I0;
	CHECKREG r4, 0xEEE1FFF2;
	CHECKREG r5, 0x88819992;
	CHECKREG r6, 0x44415552;
	CHECKREG r7, 0x44415552;

//imm32 p0, 0x00001111;
	imm32 p1, 0x22213332;
	imm32 p2, 0x44415552;
	imm32 p3, 0x66617772;
	imm32 p4, 0x88819992;
	imm32 p5, 0xaaa1bbb2;
	imm32 fp, 0xccc1ddd2;
	imm32 sp, 0xe111fff2;

// P-reg to P-reg to M reg: no stall
	P1 = P4;
	M0 = P1;
	P3 = P2;
	M1 = P3;
	P5 = P4;
	M2 = P5;
	FP = SP;
	M3 = FP;

	R4 = M3;
	R5 = M2;
	R6 = M1;
	R7 = M0;
	CHECKREG r4, 0xE111FFF2;
	CHECKREG r5, 0x88819992;
	CHECKREG r6, 0x44415552;
	CHECKREG r7, 0x88819992;

//imm32 p0, 0x00001111;
	imm32 p1, 0x22213332;
	imm32 p2, 0x44215552;
	imm32 p3, 0x66217772;
	imm32 p4, 0x88219992;
	imm32 p5, 0xaa21bbb2;
	imm32 fp, 0xcc21ddd2;
	imm32 sp, 0xee21fff2;

// P-reg to P-reg to L reg: no stall
	P1 = P0;
	L0 = P1;
	P3 = P2;
	L1 = P3;
	P5 = P4;
	L2 = P5;
	FP = SP;
	L3 = FP;

	R4 = L3;
	R5 = L2;
	R6 = L1;
	R7 = L0;
	CHECKREG r4, 0xEE21FFF2;
	CHECKREG r5, 0x88219992;
	CHECKREG r6, 0x44215552;

//imm32 p0, 0x00001111;
	imm32 p1, 0x22213332;
	imm32 p2, 0x44415532;
	imm32 p3, 0x66617732;
	imm32 p4, 0x88819932;
	imm32 p5, 0xaaa1bb32;
	imm32 fp, 0xccc1dd32;
	imm32 sp, 0xeee1ff32;

// P-reg to P-reg to B reg: no stall
	P1 = FP;
	B0 = P1;
	P3 = P2;
	B1 = P3;
	P5 = P4;
	B2 = P5;
	FP = SP;
	B3 = FP;

	R4 = B3;
	R5 = B2;
	R6 = B1;
	R7 = B0;
	CHECKREG r4, 0xEEE1FF32;
	CHECKREG r5, 0x88819932;
	CHECKREG r6, 0x44415532;
	CHECKREG r7, 0xccc1dd32;

	imm32 i0, 0x03001131;
	imm32 i1, 0x23223333;
	imm32 i2, 0x43445535;
	imm32 i3, 0x63667737;
	imm32 m0, 0x83889939;
	imm32 m1, 0xa3aabb3b;
	imm32 m2, 0xc3ccdd3d;
	imm32 m3, 0xe3eeff3f;

// I,M-reg to P-reg to R-reg: no stall
	P1 = I0;
	R0 = P1;
	P2 = I1;
	R1 = P2;
	P3 = I2;
	R2 = P3;
	P4 = I3;
	R3 = P4;
	P5 = M0;
	R4 = P5;
	SP = M1;
	R5 = SP;
	FP = M2;
	R6 = FP;
	FP = M3;
	R7 = FP;

	CHECKREG r0, 0x03001131;
	CHECKREG r1, 0x23223333;
	CHECKREG r2, 0x43445535;
	CHECKREG r3, 0x63667737;
	CHECKREG r4, 0x83889939;
	CHECKREG r5, 0xA3AABB3B;
	CHECKREG r6, 0xC3CCDD3D;
	CHECKREG r7, 0xE3EEFF3F;

	imm32 i0, 0x12001111;
	imm32 i1, 0x12221333;
	imm32 i2, 0x12441555;
	imm32 i3, 0x12661777;
	imm32 m0, 0x12881999;
	imm32 m1, 0x12aa1bbb;
	imm32 m2, 0x12cc1ddd;
	imm32 m3, 0x12ee1fff;

// I,M-reg to P-reg to L,B reg: no stall
	P1 = I0;
	L0 = P1;
	P1 = I1;
	L1 = P1;
	P2 = I2;
	L2 = P2;
	P3 = I3;
	L3 = P3;
	P4 = M0;
	B0 = P4;
	P5 = M1;
	B1 = P5;
	SP = M2;
	B2 = SP;
	FP = M3;
	B3 = FP;

//CHECKREG r0, 0x12001111;
	CHECKREG p1, 0x12221333;
	CHECKREG p2, 0x12441555;
	CHECKREG p3, 0x12661777;
	CHECKREG p4, 0x12881999;
	CHECKREG p5, 0x12AA1BBB;
	CHECKREG sp, 0x12CC1DDD;
	CHECKREG fp, 0x12EE1FFF;

	R0 = L3;
	R1 = L2;
	R2 = L1;
	R3 = L0;
	R4 = B3;
	R5 = B2;
	R6 = B1;
	R7 = B0;
	CHECKREG r0, 0x12661777;
	CHECKREG r1, 0x12441555;
	CHECKREG r2, 0x12221333;
	CHECKREG r3, 0x12001111;
	CHECKREG r4, 0x12EE1FFF;
	CHECKREG r5, 0x12CC1DDD;
	CHECKREG r6, 0x12AA1BBB;
	CHECKREG r7, 0x12881999;

	imm32 l0, 0x23003111;
	imm32 l1, 0x23223333;
	imm32 l2, 0x23443555;
	imm32 l3, 0x23663777;
	imm32 b0, 0x23883999;
	imm32 b0, 0x23aa3bbb;
	imm32 b0, 0x23cc3ddd;
	imm32 b0, 0x23ee3fff;

// L,B-reg to P-reg to I,M reg: no stall
	P1 = L0;
	I0 = P1;
	P1 = L1;
	I1 = P1;
	P2 = L2;
	I2 = P2;
	P3 = L3;
	I3 = P3;
	P4 = B0;
	M0 = P4;
	P5 = B1;
	M1 = P5;
	SP = B2;
	M2 = SP;
	FP = B3;
	M3 = FP;

	R0 = M3;
	R1 = M2;
	R2 = M1;
	R3 = M0;
	R4 = I3;
	R5 = I2;
	R6 = I1;
	R7 = I0;
//CHECKREG r0, 0x1EEE1FFF;
	CHECKREG p1, 0x23223333;
	CHECKREG p2, 0x23443555;
	CHECKREG p3, 0x23663777;
	CHECKREG p4, 0x23EE3FFF;
	CHECKREG p5, 0x12AA1BBB;
	CHECKREG sp, 0x12CC1DDD;
	CHECKREG fp, 0x12EE1FFF;

	CHECKREG r0, 0x12EE1FFF;
	CHECKREG r1, 0x12CC1DDD;
	CHECKREG r2, 0x12AA1BBB;
	CHECKREG r3, 0x23EE3FFF;
	CHECKREG r4, 0x23663777;
	CHECKREG r5, 0x23443555;
	CHECKREG r6, 0x23223333;
	CHECKREG r7, 0x23003111;

	pass
