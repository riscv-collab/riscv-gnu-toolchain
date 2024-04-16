# mach: bfin

.include "testutils.inc"
	start

	FP = SP;
	CALL _foo;
	pass

___main:
	RTS;

_m1:
	LINK 0;
	R7 = [ FP + 8 ];
	DBGA ( R0.L , 1 );
	DBGA ( R1.L , 2 );
	DBGA ( R7.L , 3 );
	UNLINK;
	RTS;

_m2:
	LINK 0;
	R7 = [ FP + 8 ];
	DBGA ( R0.L , 1 );
	DBGA ( R1.L , 2 );
	DBGA ( R7.L , 3 );
	[ -- SP ] = R7;
	CALL _m1;
	SP += 4;
	UNLINK;
	RTS;

_foo:
	LINK 0;
	CALL ___main;
	R7 = 3;
	[ -- SP ] = R7;
	R0 = 1;
	R1 = 2;
	CALL _m2;
	SP += 4;
	UNLINK;
	RTS;
