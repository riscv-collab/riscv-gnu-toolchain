# mach: bfin

.include "testutils.inc"
	start

// issue 124

	R0 = 0;
	R1.L = 0x80;

	A0.w = R0;
	A0.x = R1;

	A1.w = R0;
	A1.x = R1;

	_DBG A0;
	_DBG A1;

	R5 = ( A0 += A1 );

	_DBG A0;
	R7 = A0.w;	DBGA ( R7.H , 0 ); DBGA ( R7.L , 0 );
	R7 = A0.x;	DBGA ( R7.L , 0xff80 );

	pass
