//Original:/proj/frio/dv/testcases/core/c_dsp32alu_byteop3/c_dsp32alu_byteop3.dsp
// Spec Reference: dsp32alu byteop3
# mach: bfin

.include "testutils.inc"
	start

	imm32 r0, 0x15678911;
	imm32 r1, 0x2789ab1d;
	imm32 r2, 0x34445515;
	imm32 r3, 0x46667717;
	imm32 r4, 0x5567891b;
	imm32 r5, 0x6789ab1d;
	imm32 r6, 0x74445515;
	imm32 r7, 0x86667777;
	R4 = BYTEOP3P ( R1:0 , R3:2 ) (LO);
	R5 = BYTEOP3P ( R1:0 , R3:2 ) (HI);
	R6 = BYTEOP3P ( R1:0 , R3:2 ) (LO);
	R7 = BYTEOP3P ( R1:0 , R3:2 ) (HI);
	CHECKREG r4, 0x00FF0000;
	CHECKREG r5, 0xFF000000;
	CHECKREG r6, 0x00FF0000;
	CHECKREG r7, 0xFF000000;

	imm32 r0, 0x1567892b;
	imm32 r1, 0x2789ab2d;
	imm32 r2, 0x34445525;
	imm32 r3, 0x46667727;
	imm32 r4, 0x58889929;
	imm32 r5, 0x6aaabb2b;
	imm32 r6, 0x7cccdd2d;
	imm32 r7, 0x8eeeffff;
	R0 = BYTEOP3P ( R3:2 , R1:0 ) (LO);
	R1 = BYTEOP3P ( R3:2 , R1:0 ) (LO);
	R2 = BYTEOP3P ( R3:2 , R1:0 ) (HI);
	R3 = BYTEOP3P ( R3:2 , R1:0 ) (HI);
	CHECKREG r0, 0x00FF00FF;
	CHECKREG r1, 0x00FF00FF;
	CHECKREG r2, 0xFF00FF00;
	CHECKREG r3, 0x00000000;

	imm32 r0, 0x716789ab;
	imm32 r1, 0x8289abcd;
	imm32 r2, 0x93445555;
	imm32 r3, 0xa4667777;
	imm32 r4, 0xb56789ab;
	imm32 r5, 0xd689abcd;
	imm32 r6, 0xe7445555;
	imm32 r7, 0x6f661235;
	R4 = BYTEOP3P ( R1:0 , R3:2 ) (LO);
	R5 = BYTEOP3P ( R1:0 , R3:2 ) (LO);
	R6 = BYTEOP3P ( R1:0 , R3:2 ) (HI);
	R7 = BYTEOP3P ( R1:0 , R3:2 ) (HI);
	CHECKREG r4, 0x00FF0000;
	CHECKREG r5, 0x00FF0000;
	CHECKREG r6, 0xFF000000;
	CHECKREG r7, 0xFF000000;

	imm32 r0, 0x416789ab;
	imm32 r1, 0x6289abcd;
	imm32 r2, 0x43445555;
	imm32 r3, 0x64667777;
	imm32 r4, 0x456789ab;
	imm32 r5, 0x6689abcd;
	imm32 r6, 0x47445555;
	imm32 r7, 0x68667777;
	R0 = BYTEOP3P ( R3:2 , R1:0 ) (LO);
	R1 = BYTEOP3P ( R3:2 , R1:0 ) (LO);
	R2 = BYTEOP3P ( R3:2 , R1:0 ) (HI);
	R3 = BYTEOP3P ( R3:2 , R1:0 ) (HI);
	CHECKREG r0, 0x00FF00FF;
	CHECKREG r1, 0x00FF00FF;
	CHECKREG r2, 0xFF00FF00;
	CHECKREG r3, 0x00000000;

	pass
