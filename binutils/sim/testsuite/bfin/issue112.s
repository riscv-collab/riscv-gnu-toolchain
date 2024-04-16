# mach: bfin

.include "testutils.inc"
	start


	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	A0 = 0;
	A1 = 0;
	R2.H = 0xfafa;
	R2.L = 0xf5f6;
	R3.L = 0x00ff;
	A0.w = R2;
	A0.x = R3;
	R2.H = 0x7ebc;
	R2.L = 0xd051;
	R3 = 0;
	A1.w = R2;
	A1.x = R3;
	R1.H = 0x7fff;
	R1.L = 0x8000;
	R0.H = 0x8000;
	R0.L = 0x7fff;
	A1 += R0.L * R1.L (M), R0.L = ( A0 = R0.H * R1.H ) (IH);

	_DBG A1;
	R0 = A1.w;
	R1 = A1.x;
	DBGA ( R0.L , 0xffff );
	DBGA ( R0.H , 0x7fff );
	DBGA ( R1.L , 0 );

	NOP;
	NOP;
	pass
