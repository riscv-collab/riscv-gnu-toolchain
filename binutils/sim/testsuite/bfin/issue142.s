# mach: bfin

.include "testutils.inc"
	start

// load acc with values;
	imm32 R0, 0x7d647b42;
	A0.w = R0;
	R0 = 0x0000 (Z);
	A0.x = R0;

	imm32 R0, 0x7be27f50;
	A1.w = R0;
	R0 = 0x0000 (Z);
	A1.x = R0;

// load regs with values;
	I1 = 0 (X);
	I0 = 1 (X);
	imm32 R2, 0xefef1212;
	imm32 R3, 0xf23c0189;

	SAA ( R3:2 , R3:2 ) (R);

	R0 = A0.w
	CHECKREG R0, 0x7d9f7bca;
	R0 = A0.x
	CHECKREG R0, 0;
	R1 = A1.w;
	CHECKREG R1, 0x7cc28006;
	R1 = A1.x;
	CHECKREG R1, 0;

	pass
