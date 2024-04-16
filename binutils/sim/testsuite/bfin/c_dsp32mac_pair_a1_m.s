//Original:/proj/frio/dv/testcases/core/c_dsp32mac_pair_a1_m/c_dsp32mac_pair_a1_m.dsp
// Spec Reference: dsp32mac pair a1 M MNOP
# mach: bfin

.include "testutils.inc"
	start

	A1 = A0 = 0;

// The result accumulated in A1      , and stored to a reg half
	imm32 r0, 0x63547abd;
	imm32 r1, 0x86bc8ec7;
	imm32 r2, 0xa8695679;
	imm32 r3, 0x00060007;
	imm32 r4, 0xe6b86569;
	imm32 r5, 0x1A35860b;
	imm32 r6, 0x000c086d;
	imm32 r7, 0x67Be0086;
	R7 = ( A1 += R1.L * R0.L );
	P1 = A1.w;
	R1 = ( A1 -= R2.H * R3.L );
	P2 = A1.w;
	R3 = ( A1 = R7.L * R4.H );
	P3 = A1.w;
	R5 = ( A1 += R6.H * R5.H );
	P4 = A1.w;
	CHECKREG r0, 0x63547ABD;
	CHECKREG r1, 0x93734818;
	CHECKREG r2, 0xA8695679;
	CHECKREG r3, 0xE7256BA0;
	CHECKREG r4, 0xE6B86569;
	CHECKREG r5, 0xE727E098;
	CHECKREG r6, 0x000C086D;
	CHECKREG r7, 0x936E7DD6;
	CHECKREG p1, 0x936E7DD6;
	CHECKREG p2, 0x93734818;
	CHECKREG p3, 0xE7256BA0;
	CHECKREG p4, 0xE727E098;

	imm32 r0, 0x98764abd;
	imm32 r1, 0xa1bcf4c7;
	imm32 r2, 0xb1145649;
	imm32 r3, 0x0b010005;
	imm32 r4, 0xefbcbb69;
	imm32 r5, 0x123501bb;
	imm32 r6, 0x000c001b;
	imm32 r7, 0x678e0001;
	R5 = ( A1 += R1.L * R0.H ) (M), A0 = R1.L * R0.L;
	P1 = A1.w;
	R1 = ( A1 = R2.L * R3.H ) (M), A0 = R2.H * R3.L;
	P2 = A1.w;
	R3 = ( A1 -= R4.L * R5.H ) (M), A0 += R4.H * R5.H;
	P3 = A1.w;
	R1 = ( A1 += R6.L * R7.H ) (M), A0 += R6.L * R7.H;
	P4 = A1.w;
	CHECKREG r0, 0x98764ABD;
	CHECKREG r1, 0x3FE4AC0B;
	CHECKREG r2, 0xB1145649;
	CHECKREG r3, 0x3FD9C011;
	CHECKREG r4, 0xEFBCBB69;
	CHECKREG r5, 0xE078DC52;
	CHECKREG r6, 0x000C001B;
	CHECKREG r7, 0x678E0001;
	CHECKREG p1, 0xE078DC52;
	CHECKREG p2, 0x03B57949;
	CHECKREG p3, 0x3FD9C011;
	CHECKREG p4, 0x3FE4AC0B;

	imm32 r0, 0x7136459d;
	imm32 r1, 0xabd69ec7;
	imm32 r2, 0x71145679;
	imm32 r3, 0xd8010007;
	imm32 r4, 0xeddc1569;
	imm32 r5, 0x122d010b;
	imm32 r6, 0x0003d01d;
	imm32 r7, 0x678e0d61;
	R5 = A1 , A0 = R1.L * R0.L;
	P1 = A1.w;
	R7 = A1 , A0 -= R2.H * R3.L;
	P2 = A1.w;
	R1 = A1 , A0 += R4.H * R5.H;
	P3 = A1.w;
	R5 = A1 , A0 += R6.L * R7.H;
	P4 = A1.w;
	CHECKREG r0, 0x7136459D;
	CHECKREG r1, 0x3FE4AC0B;
	CHECKREG r2, 0x71145679;
	CHECKREG r3, 0xD8010007;
	CHECKREG r4, 0xEDDC1569;
	CHECKREG r5, 0x3FE4AC0B;
	CHECKREG r6, 0x0003D01D;
	CHECKREG r7, 0x3FE4AC0B;
	CHECKREG p1, 0x3FE4AC0B;
	CHECKREG p2, 0x3FE4AC0B;
	CHECKREG p3, 0x3FE4AC0B;
	CHECKREG p4, 0x3FE4AC0B;

	imm32 r0, 0x123489bd;
	imm32 r1, 0x91bcfec7;
	imm32 r2, 0xa9145679;
	imm32 r3, 0xd0910007;
	imm32 r4, 0x34567899;
	imm32 r5, 0xd235910b;
	imm32 r6, 0x0d0c0999;
	imm32 r7, 0x67de0009;
	R1 = ( A1 += R5.H * R3.H ) (M);
	P1 = A1.w;
	R3 = ( A1 = R2.H * R1.H ) (M);
	P2 = A1.w;
	R5 = ( A1 -= R7.H * R0.H ) (M);
	P3 = A1.w;
	R7 = ( A1 += R4.H * R6.H ) (M);
	P4 = A1.w;
	CHECKREG r0, 0x123489BD;
	CHECKREG r1, 0x1A95CC10;
	CHECKREG r2, 0xA9145679;
	CHECKREG r3, 0xF6F970A4;
	CHECKREG r4, 0x34567899;
	CHECKREG r5, 0xEF96BB8C;
	CHECKREG r6, 0x0D0C0999;
	CHECKREG r7, 0xF2418D94;
	CHECKREG p1, 0x1A95CC10;
	CHECKREG p2, 0xF6F970A4;
	CHECKREG p3, 0xEF96BB8C;
	CHECKREG p4, 0xF2418D94;

	pass
