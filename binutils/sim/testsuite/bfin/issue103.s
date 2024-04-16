# mach: bfin

.include "testutils.inc"
	start

	A0 = 0;
	A1 = 0;
	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R2.H = 0xf12e;
	R2.L = 0xbeaa;
	R3.L = 0x00ff;
	A1.w = R2;
	A1.x = R3;
	R0.H = 0xd136;
	R0.L = 0x459d;
	R1.H = 0xabd6;
	R1.L = 0x9ec7;

	_DBG A1;
	R5 = A1 , A0 = R1.L * R0.L (FU);

	DBGA ( R5.H , 0xffff );
	DBGA ( R5.L , 0xffff );

	NOP;
	NOP;
	NOP;
	NOP;
	pass
