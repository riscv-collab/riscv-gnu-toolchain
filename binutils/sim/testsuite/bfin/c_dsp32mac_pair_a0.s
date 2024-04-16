//Original:/proj/frio/dv/testcases/core/c_dsp32mac_pair_a0/c_dsp32mac_pair_a0.dsp
// Spec Reference: dsp32mac pair a0
# mach: bfin

.include "testutils.inc"
	start

	A1 = A0 = 0;

// The result accumulated in A       , and stored to a reg half
	imm32 r0, 0x63545abd;
	imm32 r1, 0x86bcfec7;
	imm32 r2, 0xa8645679;
	imm32 r3, 0x00860007;
	imm32 r4, 0xefb86569;
	imm32 r5, 0x1235860b;
	imm32 r6, 0x000c086d;
	imm32 r7, 0x678e0086;
	A1 += R1.L * R0.L, R6 = ( A0 = R1.L * R0.L );
	P1 = A1.w;
	P5 = A0.w;
	A1 = R2.L * R3.L, R0 = ( A0 = R2.H * R3.L );
	P2 = A1.w;
	A1 -= R7.L * R4.L, R2 = ( A0 += R7.H * R4.H );
	P3 = A0.w;
	A1 += R6.L * R5.L, R4 = ( A0 += R6.L * R5.H );
	P4 = A0.w;
	CHECKREG r0, 0xFFFB3578;
	CHECKREG r1, 0x86BCFEC7;
	CHECKREG r2, 0xF2CF3598;
	CHECKREG r3, 0x00860007;
	CHECKREG r4, 0xF70DA834;
	CHECKREG r5, 0x1235860B;
	CHECKREG r6, 0xFF221DD6;
	CHECKREG r7, 0x678E0086;
	CHECKREG p1, 0xFF221DD6;
	CHECKREG p2, 0x0004BA9E;
	CHECKREG p3, 0xF2CF3598;
	CHECKREG p4, 0xF70DA834;
	CHECKREG p5, 0xFF221DD6;

	imm32 r0, 0x98764abd;
	imm32 r1, 0xa1bcf4c7;
	imm32 r2, 0xa1145649;
	imm32 r3, 0x00010005;
	imm32 r4, 0xefbc1569;
	imm32 r5, 0x1235010b;
	imm32 r6, 0x000c001d;
	imm32 r7, 0x678e0001;
	A1 += R1.L * R0.H, R4 = ( A0 -= R1.L * R0.L );
	P1 = A0.w;
	A1 = R2.L * R3.H, R0 = ( A0 = R2.H * R3.L );
	P2 = A0.w;
	A1 -= R4.L * R5.H, R2 = ( A0 += R4.H * R5.H );
	P3 = A0.w;
	A1 += R6.L * R7.H, R0 = ( A0 += R6.L * R7.H );
	P4 = A0.w;
	CHECKREG r0, 0xFFBC8F22;
	CHECKREG r1, 0xA1BCF4C7;
	CHECKREG r2, 0xFFA518F6;
	CHECKREG r3, 0x00010005;
	CHECKREG r4, 0xFD9B2E5E;
	CHECKREG r5, 0x1235010B;
	CHECKREG r6, 0x000C001D;
	CHECKREG r7, 0x678E0001;
	CHECKREG p1, 0xFD9B2E5E;
	CHECKREG p2, 0xFFFC4AC8;
	CHECKREG p3, 0xFFA518F6;
	CHECKREG p4, 0xFFBC8F22;

	imm32 r0, 0x7136459d;
	imm32 r1, 0xabd69ec7;
	imm32 r2, 0x71145679;
	imm32 r3, 0x08010007;
	imm32 r4, 0xef9c1569;
	imm32 r5, 0x1225010b;
	imm32 r6, 0x0003401d;
	imm32 r7, 0x678e0561;
	A1 += R1.H * R0.L, R4 = ( A0 = R1.L * R0.L );
	P1 = A0.w;
	A1 = R2.H * R3.L, R6 = ( A0 = R2.H * R3.L );
	P2 = A0.w;
	A1 -= R4.H * R5.L, R0 = ( A0 += R4.H * R5.H );
	P3 = A0.w;
	A1 += R6.H * R7.L, R4 = ( A0 += R6.L * R7.H );
	P4 = A0.w;
	CHECKREG r0, 0xF8876658;
	CHECKREG r1, 0xABD69EC7;
	CHECKREG r2, 0x71145679;
	CHECKREG r3, 0x08010007;
	CHECKREG r4, 0x1EA0F4F8;
	CHECKREG r5, 0x1225010B;
	CHECKREG r6, 0x00062F18;
	CHECKREG r7, 0x678E0561;
	CHECKREG p1, 0xCB200616;
	CHECKREG p2, 0x00062F18;
	CHECKREG p3, 0xF8876658;
	CHECKREG p4, 0x1EA0F4F8;

	imm32 r0, 0x123489bd;
	imm32 r1, 0x91bcfec7;
	imm32 r2, 0xa9145679;
	imm32 r3, 0xd0910007;
	imm32 r4, 0xedb91569;
	imm32 r5, 0xd235910b;
	imm32 r6, 0x0d0c0999;
	imm32 r7, 0x67de0009;
	A1 += R5.H * R3.H, R0 = ( A0 = R5.L * R3.L );
	P1 = A0.w;
	A1 -= R2.H * R1.H, R2 = ( A0 -= R2.H * R1.L );
	P2 = A0.w;
	A1 = R7.H * R0.H, R4 = ( A0 += R7.H * R0.H );
	P3 = A0.w;
	A1 += R4.H * R6.H, R6 = ( A0 += R4.L * R6.H );
	P4 = A0.w;
	CHECKREG r0, 0xFFF9EE9A;
	CHECKREG r1, 0x91BCFEC7;
	CHECKREG r2, 0xFF256182;
	CHECKREG r3, 0xD0910007;
	CHECKREG r4, 0xFF1FB35E;
	CHECKREG r5, 0xD235910B;
	CHECKREG r6, 0xF750102E;
	CHECKREG r7, 0x67DE0009;
	CHECKREG p1, 0xFFF9EE9A;
	CHECKREG p2, 0xFF256182;
	CHECKREG p3, 0xFF1FB35E;
	CHECKREG p4, 0xF750102E;

	pass
