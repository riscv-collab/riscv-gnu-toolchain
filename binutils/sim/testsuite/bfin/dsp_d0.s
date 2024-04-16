# mach: bfin

.include "testutils.inc"
	start

	loadsym I0, vec;

	R0 = [ I0 ++ ];
	DBGA ( R0.L , 1 );	DBGA ( R0.H , 2 );
	R0 = [ I0 ++ ];
	DBGA ( R0.L , 2 );	DBGA ( R0.H , 3 );

	loadsym I3, vec;
	R0 = 4;
	M1 = R0;

	_DBG I3;
	R0 = [ I3 ++ M1 ];
	DBGA ( R0.L , 1 );	DBGA ( R0.H , 2 );
	_DBG I3;
	R0 = [ I3 ++ M1 ];
	DBGA ( R0.L , 2 );	DBGA ( R0.H , 3 );

	pass

	.data
vec:
	.dw 1
	.dw 2
	.dw 2
	.dw 3
