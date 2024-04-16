# mach: bfin

.include "testutils.inc"
	start

	A0 = 0;
	R0.L = 0x10;
	A0.x = R0;

	R0.L = 0x0038;
	R0.H = 0x0006;

	R0.L = SIGNBITS A0;

	DBGA ( R0.L , 0xfffa );
	DBGA ( R0.H , 0x0006 );

	pass
