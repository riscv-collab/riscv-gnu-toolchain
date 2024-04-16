//Original:/proj/frio/dv/testcases/core/c_dsp32mac_pair_a1/c_dsp32mac_pair_a1.dsp
// Spec Reference: dsp32mac pair a1
# mach: bfin

.include "testutils.inc"
	start

	A1 = A0 = 0;

// The result accumulated in A1      , and stored to a reg half
	imm32 r0, 0x63545abd;
	imm32 r1, 0x86bcfec7;
	imm32 r2, 0xa8645679;
	imm32 r3, 0x00860007;
	imm32 r4, 0xefb86569;
	imm32 r5, 0x1235860b;
	imm32 r6, 0x000c086d;
	imm32 r7, 0x678e0086;
	R7 = ( A1 += R1.L * R0.L ), A0 = R1.L * R0.L;
	P1 = A1.w;
	R1 = ( A1 = R2.L * R3.L ), A0 += R2.H * R3.L;
	P2 = A1.w;
	R3 = ( A1 -= R7.L * R4.L ), A0 += R7.H * R4.H;
	P3 = A1.w;
	R5 = ( A1 -= R6.L * R5.L ), A0 -= R6.L * R5.H;
	P4 = A1.w;
	CHECKREG r0, 0x63545ABD;
	CHECKREG r1, 0x0004BA9E;
	CHECKREG r2, 0xA8645679;
	CHECKREG r3, 0xE8616512;
	CHECKREG r4, 0xEFB86569;
	CHECKREG r5, 0xF0688FB4;
	CHECKREG r6, 0x000C086D;
	CHECKREG r7, 0xFF221DD6;
	CHECKREG p1, 0xFF221DD6;
	CHECKREG p2, 0x0004BA9E;
	CHECKREG p3, 0xE8616512;
	CHECKREG p4, 0xF0688FB4;

	imm32 r0, 0x98764abd;
	imm32 r1, 0xa1bcf4c7;
	imm32 r2, 0xa1145649;
	imm32 r3, 0x00010005;
	imm32 r4, 0xefbc1569;
	imm32 r5, 0x1235010b;
	imm32 r6, 0x000c001d;
	imm32 r7, 0x678e0001;
	R5 = ( A1 += R1.L * R0.H ), A0 -= R1.L * R0.L;
	P1 = A1.w;
	R1 = ( A1 = R2.L * R3.H ), A0 -= R2.H * R3.L;
	P2 = A1.w;
	R3 = ( A1 -= R4.L * R5.H ), A0 += R4.H * R5.H;
	P3 = A1.w;
	R1 = ( A1 += R6.L * R7.H ), A0 += R6.L * R7.H;
	P4 = A1.w;
	CHECKREG r0, 0x98764ABD;
	CHECKREG r1, 0x012F2306;
	CHECKREG r2, 0xA1145649;
	CHECKREG r3, 0x0117ACDA;
	CHECKREG r4, 0xEFBC1569;
	CHECKREG r5, 0xF97C8728;
	CHECKREG r6, 0x000C001D;
	CHECKREG r7, 0x678E0001;
	CHECKREG p1, 0xF97C8728;
	CHECKREG p2, 0x0000AC92;
	CHECKREG p3, 0x0117ACDA;
	CHECKREG p4, 0x012F2306;

	imm32 r0, 0x7136459d;
	imm32 r1, 0xabd69ec7;
	imm32 r2, 0x71145679;
	imm32 r3, 0x08010007;
	imm32 r4, 0xef9c1569;
	imm32 r5, 0x1225010b;
	imm32 r6, 0x0003401d;
	imm32 r7, 0x678e0561;
	R5 = ( A1 += R1.H * R0.L ), A0 = R1.L * R0.L;
	P1 = A1.w;
	R7 = ( A1 -= R2.H * R3.L ), A0 -= R2.H * R3.L;
	P2 = A1.w;
	R1 = ( A1 += R4.H * R5.L ), A0 -= R4.H * R5.H;
	P3 = A1.w;
	R5 = ( A1 += R6.H * R7.L ), A0 += R6.L * R7.H;
	P4 = A1.w;
	CHECKREG r0, 0x7136459D;
	CHECKREG r1, 0xCABE16DA;
	CHECKREG r2, 0x71145679;
	CHECKREG r3, 0x08010007;
	CHECKREG r4, 0xEF9C1569;
	CHECKREG r5, 0xCABE9156;
	CHECKREG r6, 0x0003401D;
	CHECKREG r7, 0xD363146A;
	CHECKREG p1, 0xD3694382;
	CHECKREG p2, 0xD363146A;
	CHECKREG p3, 0xCABE16DA;
	CHECKREG p4, 0xCABE9156;

	imm32 r0, 0x123489bd;
	imm32 r1, 0x91bcfec7;
	imm32 r2, 0xa9145679;
	imm32 r3, 0xd0910007;
	imm32 r4, 0xedb91569;
	imm32 r5, 0xd235910b;
	imm32 r6, 0x0d0c0999;
	imm32 r7, 0x67de0009;
	R1 = ( A1 += R5.H * R3.H ), A0 = R5.L * R3.L;
	P1 = A1.w;
	R3 = ( A1 = R2.H * R1.H ), A0 -= R2.H * R1.L;
	P2 = A1.w;
	R5 = ( A1 -= R7.H * R0.H ), A0 += R7.H * R0.H;
	P3 = A1.w;
	R7 = ( A1 += R4.H * R6.H ), A0 += R4.L * R6.H;
	P4 = A1.w;
	CHECKREG r0, 0x123489BD;
	CHECKREG r1, 0xDBB6D160;
	CHECKREG r2, 0xA9145679;
	CHECKREG r3, 0x18A4A070;
	CHECKREG r4, 0xEDB91569;
	CHECKREG r5, 0x09DF3640;
	CHECKREG r6, 0x0D0C0999;
	CHECKREG r7, 0x08024998;
	CHECKREG p1, 0xDBB6D160;
	CHECKREG p2, 0x18A4A070;
	CHECKREG p3, 0x09DF3640;
	CHECKREG p4, 0x08024998;

	pass
