# mach: bfin

.include "testutils.inc"
	start

	a0=0;
	R0.L = 1;
	R0.H = 0;
	R0 *= R0;
	_DBG R0;
	_DBG A0;

	R7 = A0.w;
	DBGA ( R7.H , 0 );	DBGA ( R7.L , 0 );

	R0.L = -1;
	R0.H = 32767;

	_DBG R0;

	a0=0;
	R0 *= R0;

	_DBG R0;
	_DBG A0;
	R7 = A0.w;
	DBGA ( R7.H , 0 );	DBGA ( R7.L , 0 );
	R7 = A0.x;
	DBGA ( R7.L , 0x0 );

	pass
