//Original:/proj/frio/dv/testcases/core/c_dsp32mac_pair_a1a0_m/c_dsp32mac_pair_a1a0_m.dsp
// Spec Reference: dsp32mac pair a1a0 M MNOP
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
	R7 = ( A1 += R0.L * R1.L ) (M),  R6 = ( A0 = R0.L * R1.L )  (IS);
	P1 = A1.w;
	P2 = A0.w;
	R1 = ( A1 = R3.L * R2.L ) (M),  R0 = ( A0 = R3.H * R2.L )  (IS);
	P3 = A1.w;
	P4 = A0.w;
	R3 = ( A1 -= R7.L * R6.L ) (M),  R2 = ( A0 += R7.H * R6.H )  (IS);
	P5 = A1.w;
	SP = A0.w;
	R5 = ( A1 += R5.L * R4.L ) (M),  R4 = ( A0 += R5.L * R4.H )  (IS);
	FP = A0.w;
	CHECKREG r0, 0x002D4356;
	CHECKREG r1, 0x00025D4F;
	CHECKREG r2, 0x00061B84;
	CHECKREG r3, 0xFF23D196;
	CHECKREG r4, 0x07C7B86C;
	CHECKREG r5, 0xCED42319;
	CHECKREG r6, 0xFF910EEB;
	CHECKREG r7, 0x5A4E0EEB;
	CHECKREG p1, 0x5A4E0EEB;
	CHECKREG p2, 0xFF910EEB;
	CHECKREG p3, 0x00025D4F;
	CHECKREG p4, 0x002D4356;
	CHECKREG p5, 0xFF23D196;
	CHECKREG sp, 0x00061B84;
	CHECKREG fp, 0x07C7B86C;

	imm32 r0, 0x98764abd;
	imm32 r1, 0xa1bcf4c7;
	imm32 r2, 0xa1145649;
	imm32 r3, 0x00010005;
	imm32 r4, 0xefbc1569;
	imm32 r5, 0x1235010b;
	imm32 r6, 0x000c001d;
	imm32 r7, 0x678e0001;
	R5 = A1,  R4 = ( A0 = R3.L * R1.L )  (IS);
	P1 = A1.w;
	P2 = A0.w;
	R1 = A1,  R0 = ( A0 -= R0.H * R5.L )  (IS);
	P3 = A1.w;
	P4 = A0.w;
	R3 = A1,  R2 = ( A0 += R2.H * R7.H )  (IS);
	P5 = A1.w;
	SP = A0.w;
	R1 = A1,  R0 = ( A0 -= R4.L * R6.H )  (IS);
	FP = A1.w;
	CHECKREG r0, 0xE7CEC8D1;
	CHECKREG r1, 0xCED42319;
	CHECKREG r2, 0xE7CC2775;
	CHECKREG r3, 0xCED42319;
	CHECKREG r4, 0xFFFFC7E3;
	CHECKREG r5, 0xCED42319;
	CHECKREG r6, 0x000C001D;
	CHECKREG r7, 0x678E0001;
	CHECKREG p1, 0xCED42319;
	CHECKREG p2, 0xFFFFC7E3;
	CHECKREG p3, 0xCED42319;
	CHECKREG p4, 0x0E31C25D;
	CHECKREG p5, 0xCED42319;
	CHECKREG sp, 0xE7CC2775;
	CHECKREG fp, 0xCED42319;

	imm32 r0, 0x7136459d;
	imm32 r1, 0xabd69ec7;
	imm32 r2, 0x71145679;
	imm32 r3, 0x08010007;
	imm32 r4, 0xef9c1569;
	imm32 r5, 0x1225010b;
	imm32 r6, 0x0003401d;
	imm32 r7, 0x678e0561;
	R5 = ( A1 += R4.H * R3.L ) (M),  R4 = ( A0 = R4.L * R3.L )  (IS);
	P1 = A1.w;
	P2 = A0.w;
	R7 = A1,  R6 = ( A0 = R5.H * R0.L )  (IS);
	P3 = A1.w;
	P4 = A0.w;
	R1 = ( A1 = R2.H * R6.L ) (M),  R0 = ( A0 += R2.H * R6.H )  (IS);
	P5 = A1.w;
	SP = A0.w;
	R5 = A1,  R4 = ( A0 += R7.L * R1.H )  (IS);
	FP = A1.w;
	CHECKREG r0, 0xECB84AE7;
	CHECKREG r1, 0x5091B70C;
	CHECKREG r2, 0x71145679;
	CHECKREG r3, 0x08010007;
	CHECKREG r4, 0xD3A83F94;
	CHECKREG r5, 0x5091B70C;
	CHECKREG r6, 0xF2A0B667;
	CHECKREG r7, 0xCED3B05D;
	CHECKREG p1, 0xCED3B05D;
	CHECKREG p2, 0x000095DF;
	CHECKREG p3, 0xCED3B05D;
	CHECKREG p4, 0xF2A0B667;
	CHECKREG p5, 0x5091B70C;
	CHECKREG sp, 0xECB84AE7;
	CHECKREG fp, 0x5091B70C;

	imm32 r0, 0x123489bd;
	imm32 r1, 0x91bcfec7;
	imm32 r2, 0xa9145679;
	imm32 r3, 0xd0910007;
	imm32 r4, 0xedb91569;
	imm32 r5, 0xd235910b;
	imm32 r6, 0x0d0c0999;
	imm32 r7, 0x67de0009;
	R1 = A1,  R0 = ( A0 = R5.L * R2.L )  (IS);
	P1 = A1.w;
	P2 = A0.w;
	R3 = ( A1 = R3.H * R1.H ) (M),  R2 = ( A0 -= R3.H * R1.L )  (IS);
	P3 = A1.w;
	P4 = A0.w;
	R5 = ( A1 = R7.H * R0.H ) (M),  R4 = ( A0 += R7.H * R0.H )  (IS);
	P5 = A0.w;
	SP = A1.w;
	R7 = A1,  R6 = ( A0 += R4.L * R6.H )  (IS);
	FP = A0.w;
	CHECKREG r0, 0xDA854033;
	CHECKREG r1, 0x5091B70C;
	CHECKREG r2, 0xCD00D267;
	CHECKREG r3, 0xF1127221;
	CHECKREG r4, 0xBDCBD4BD;
	CHECKREG r5, 0x58A90256;
	CHECKREG r6, 0xBB976699;
	CHECKREG r7, 0x58A90256;
	CHECKREG p1, 0x5091B70C;
	CHECKREG p2, 0xDA854033;
	CHECKREG p3, 0xF1127221;
	CHECKREG p4, 0xCD00D267;
	CHECKREG p5, 0xBDCBD4BD;
	CHECKREG sp, 0x58A90256;
	CHECKREG fp, 0xBB976699;

	pass
