//Original:/proj/frio/dv/testcases/core/c_dsp32alu_saa/c_dsp32alu_saa.dsp
// Spec Reference: dsp32alu saa
# mach: bfin

.include "testutils.inc"
	start

	A1 = 0;
	A0 = 0;

	imm32 r0, 0x15678911;
	imm32 r1, 0x2789ab1d;
	imm32 r2, 0x34445515;
	imm32 r3, 0x46667717;
	imm32 r4, 0x5567891b;
	imm32 r5, 0x6789ab1d;
	imm32 r6, 0x74445515;
	imm32 r7, 0x86667777;
	A0 = 0;
	A1 = 0;
	SAA ( R1:0 , R3:2 );
	R4 = A0.w;
	R5 = A1.w;
	CHECKREG r4, 0x00340004;
	CHECKREG r5, 0x001F0023;
	SAA ( R3:2 , R1:0 );
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r6, 0x00680008;
	CHECKREG r7, 0x003E0046;

	imm32 r0, 0x1567892b;
	imm32 r1, 0x2789ab2d;
	imm32 r2, 0x34445525;
	imm32 r3, 0x46667727;
	imm32 r4, 0x00340004;
	imm32 r5, 0x001F0023;
	imm32 r6, 0x00680008;
	imm32 r7, 0x003E0046;
	SAA ( R1:0 , R3:2 );
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG r0, 0x009C000E;
	CHECKREG r1, 0x005D0069;
	SAA ( R3:2 , R1:0 );
	R2 = A0.w;
	R3 = A1.w;
	CHECKREG r2, 0x00F10025;
	CHECKREG r3, 0x009100C1;

	imm32 r0, 0x496789ab;
	imm32 r1, 0x6489abcd;
	imm32 r2, 0x4b445555;
	imm32 r3, 0x6c647777;
	imm32 r4, 0x8d889999;
	imm32 r5, 0xaeaa4bbb;
	imm32 r6, 0xcfccd44d;
	imm32 r7, 0xe1eefff4;
	SAA ( R3:2 , R1:0 ) (R);
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG r0, 0x0125007B;
	CHECKREG r1, 0x009900E6;
	SAA ( R1:0 , R3:2 ) (R);
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r6, 0x019C00EA;
	CHECKREG r7, 0x0105011B;

	pass
