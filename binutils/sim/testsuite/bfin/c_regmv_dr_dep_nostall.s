//Original:/proj/frio/dv/testcases/core/c_regmv_dr_dep_nostall/c_regmv_dr_dep_nostall.dsp
// Spec Reference: regmv dr-dep no stall
# mach: bfin

.include "testutils.inc"
	start

	imm32 r0, 0x00000001;
	imm32 r1, 0x00110001;
	imm32 r2, 0x00220002;
	imm32 r3, 0x00330003;
	imm32 r4, 0x00440004;
	imm32 r5, 0x00550005;
	imm32 r6, 0x00660006;
	imm32 r7, 0x00770007;
// R-reg to R-reg: no stall
	R0 = R0;
	R1 = R0;
	R2 = R1;
	R3 = R2;
	R4 = R3;
	R5 = R4;
	R6 = R5;
	R7 = R6;
	R0 = R7;

	CHECKREG r0, 0x00000001;
	CHECKREG r1, 0x00000001;
	CHECKREG r2, 0x00000001;
	CHECKREG r3, 0x00000001;
	CHECKREG r4, 0x00000001;
	CHECKREG r5, 0x00000001;
	CHECKREG r6, 0x00000001;
	CHECKREG r7, 0x00000001;

//imm32 p0, 0x00001111;
	imm32 p1, 0x22223333;
	imm32 p2, 0x44445555;
	imm32 p3, 0x66667777;
	imm32 p4, 0x88889999;
	imm32 p5, 0xaaaabbbb;
	imm32 fp, 0xccccdddd;
	imm32 sp, 0xeeeeffff;

// P-reg to R-reg to I,M reg: no stall
	R0 = P0;
	I0 = R0;
	R1 = P1;
	I1 = R1;
	R2 = P2;
	I2 = R2;
	R3 = P3;
	I3 = R3;
	R4 = P4;
	M0 = R4;
	R5 = P5;
	M1 = R5;
	R6 = FP;
	M2 = R6;
	R7 = SP;
	M3 = R7;

	CHECKREG r1, 0x22223333;
	CHECKREG r2, 0x44445555;
	CHECKREG r3, 0x66667777;
	CHECKREG r4, 0x88889999;
	CHECKREG r5, 0xAAAABBBB;
	CHECKREG r6, 0xCCCCDDDD;
	CHECKREG r7, 0xEEEEFFFF;

	R0 = M3;
	R1 = M2;
	R2 = M1;
	R3 = M0;
	R4 = I3;
	R5 = I2;
	R6 = I1;
	R7 = I0;
	CHECKREG r0, 0xEEEEFFFF;
	CHECKREG r1, 0xCCCCDDDD;
	CHECKREG r2, 0xAAAABBBB;
	CHECKREG r3, 0x88889999;
	CHECKREG r4, 0x66667777;
	CHECKREG r5, 0x44445555;
	CHECKREG r6, 0x22223333;

	imm32 i0, 0x00001111;
	imm32 i1, 0x22223333;
	imm32 i2, 0x44445555;
	imm32 i3, 0x66667777;
	imm32 m0, 0x88889999;
	imm32 m0, 0xaaaabbbb;
	imm32 m0, 0xccccdddd;
	imm32 m0, 0xeeeeffff;

// I,M-reg to R-reg to P-reg: no stall
	R0 = I0;
	P1 = R0;
	R1 = I1;
	P1 = R1;
	R2 = I2;
	P2 = R2;
	R3 = I3;
	P3 = R3;
	R4 = M0;
	P4 = R4;
	R5 = M1;
	P5 = R5;
	R6 = M2;
	SP = R6;
	R7 = M3;
	FP = R7;

	CHECKREG p1, 0x22223333;
	CHECKREG p2, 0x44445555;
	CHECKREG p3, 0x66667777;
	CHECKREG p4, 0xEEEEFFFF;
	CHECKREG p5, 0xAAAABBBB;
	CHECKREG sp, 0xCCCCDDDD;
	CHECKREG fp, 0xEEEEFFFF;

	imm32 i0, 0x10001111;
	imm32 i1, 0x12221333;
	imm32 i2, 0x14441555;
	imm32 i3, 0x16661777;
	imm32 m0, 0x18881999;
	imm32 m1, 0x1aaa1bbb;
	imm32 m2, 0x1ccc1ddd;
	imm32 m3, 0x1eee1fff;

// I,M-reg to R-reg to L,B reg: no stall
	R0 = I0;
	L0 = R0;
	R1 = I1;
	L1 = R1;
	R2 = I2;
	L2 = R2;
	R3 = I3;
	L3 = R3;
	R4 = M0;
	B0 = R4;
	R5 = M1;
	B1 = R5;
	R6 = M2;
	B2 = R6;
	R7 = M3;
	B3 = R7;

	CHECKREG r0, 0x10001111;
	CHECKREG r1, 0x12221333;
	CHECKREG r2, 0x14441555;
	CHECKREG r3, 0x16661777;
	CHECKREG r4, 0x18881999;
	CHECKREG r5, 0x1AAA1BBB;
	CHECKREG r6, 0x1CCC1DDD;
	CHECKREG r7, 0x1EEE1FFF;

	R0 = L3;
	R1 = L2;
	R2 = L1;
	R3 = L0;
	R4 = B3;
	R5 = B2;
	R6 = B1;
	R7 = B0;
	CHECKREG r0, 0x16661777;
	CHECKREG r1, 0x14441555;
	CHECKREG r2, 0x12221333;
	CHECKREG r3, 0x10001111;
	CHECKREG r4, 0x1EEE1FFF;
	CHECKREG r5, 0x1CCC1DDD;
	CHECKREG r6, 0x1AAA1BBB;
	CHECKREG r7, 0x18881999;

	imm32 l0, 0x20003111;
	imm32 l1, 0x22223333;
	imm32 l2, 0x24443555;
	imm32 l3, 0x26663777;
	imm32 b0, 0x28883999;
	imm32 b0, 0x2aaa3bbb;
	imm32 b0, 0x2ccc3ddd;
	imm32 b0, 0x2eee3fff;

// L,B-reg to R-reg to I,M reg: no stall
	R0 = L0;
	I0 = R0;
	R1 = L1;
	I1 = R1;
	R2 = L2;
	I2 = R2;
	R3 = L3;
	I3 = R3;
	R4 = B0;
	M0 = R4;
	R5 = B1;
	M1 = R5;
	R6 = B2;
	M2 = R6;
	R7 = B3;
	M3 = R7;

	R0 = M3;
	R1 = M2;
	R2 = M1;
	R3 = M0;
	R4 = I3;
	R5 = I2;
	R6 = I1;
	R7 = I0;
	CHECKREG r0, 0x1EEE1FFF;
	CHECKREG r1, 0x1CCC1DDD;
	CHECKREG r2, 0x1AAA1BBB;
	CHECKREG r3, 0x2EEE3FFF;
	CHECKREG r4, 0x26663777;
	CHECKREG r5, 0x24443555;
	CHECKREG r6, 0x22223333;
	CHECKREG r7, 0x20003111;

	imm32 r0, 0x00000030;
	imm32 r1, 0x00000031;
	imm32 r2, 0x00000003;
	imm32 r3, 0x00330003;
	imm32 r4, 0x00440004;
	imm32 r5, 0x00550005;
	imm32 r6, 0x00660006;
	imm32 r7, 0x00770007;

// R-reg to R-reg to sysreg to Reg: no stall
	R3 = R0;
	ASTAT = R3;
	R6 = ASTAT;
	R4 = R1;
	RETS = R4;
	R7 = RETS;

	CHECKREG r0, 0x00000030;
	CHECKREG r1, 0x00000031;
	CHECKREG r2, 0x00000003;
	CHECKREG r3, 0x00000030;
	CHECKREG r4, 0x00000031;
	CHECKREG r5, 0x00550005;
	CHECKREG r6, 0x00000030;
	CHECKREG r7, 0x00000031;

	pass
