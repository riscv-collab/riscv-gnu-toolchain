# mach: bfin

.include "testutils.inc"
	start

	R0 = 0x1111 (X);
	R0.H = 0x1111;
	A0.x = R0;
	R1 = A0.x;
	DBGA ( R1.L , 0x11 );
	DBGA ( R1.H , 0x0 );
	pass
