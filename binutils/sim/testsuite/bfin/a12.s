//  Test   SAA
# mach: bfin

.include "testutils.inc"
	start

	I0 = 0;
	I1 = 0;

	imm32 R0, 0x04030201;
	imm32 R2, 0x04030201;
	A1 = A0 = 0;
	saa(r1:0,r3:2);
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG R0, 0;
	CHECKREG R1, 0;

	imm32 R0, 0x00000201;
	imm32 R2, 0x00020102;
	A1 = A0 = 0;
	saa(r1:0,r3:2);
	saa(r1:0,r3:2);
	saa(r1:0,r3:2);
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG R0, 0x00030003;
	CHECKREG R1, 0x00000006;

	imm32 R0, 0x000300ff;
	imm32 R2, 0x0001ff00;
	A1 = A0 = 0;
	saa(r1:0,r3:2);
	saa(r1:0,r3:2);
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG R0, 0x1fe01fe;
	CHECKREG R1, 0x0000004;

	pass
