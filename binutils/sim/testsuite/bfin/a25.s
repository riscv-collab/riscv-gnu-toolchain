# mach: bfin

.include "testutils.inc"
	start


	A1 = A0 = 0;
	R0.L = 0x01;
	A0.x = R0;
//A0 = 0x0100000000
//A1 = 0x0000000000

	R4.L = 0x2d1a;
	R4.H = 0x32e0;

	A1.x = R4;
//A1 = 0x1a00000000

	A0.w = A1.x;

	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x001a );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0001 );

	pass
