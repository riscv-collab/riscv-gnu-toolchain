# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	I0 = 0x1100 (X);
	L0 = 0x10c0 (X);
	M0 = 0 (X);
	B0 = 0 (X);
	I0 += M0;
	R0 = I0;
	DBGA ( R0.L , 0x40 );

	pass
