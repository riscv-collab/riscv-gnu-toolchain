# mach: bfin

.include "testutils.inc"
	start

	I1 = 0x4 (X);
	B1 = 0x0 (X);
	L1 = 0x10 (X);
	M0 = 8 (X);
	I1 -= M0;
	R0 = I1;
	DBGA ( R0.L , 0xc );

	I1 = 0xf0 (X);
	B1 = 0x100 (X);
	L1 = 0x10 (X);
	M0 = 2 (X);
	I1 += M0;
	R0 = I1;
	DBGA ( R0.L , 0xf2 );

	I2 = 0x1000 (X);
	B2.L = 0;
	B2.H = 0x9000;
	L2 = 0x10 (X);
	M2 = 0 (X);
	I2 += M2;
	R0 = I2;
	DBGA ( R0.L , 0x1000 );

	pass
