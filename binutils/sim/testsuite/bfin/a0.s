# mach: bfin

.include "testutils.inc"
	start

	R0 = 1;
	R0 <<= 1;
	DBGA ( R0.L , 2 );
	R0 <<= 1;
	DBGA ( R0.L , 4 );
	R0 <<= 3;
	DBGA ( R0.L , 32 );
	R0 += 5;
	DBGA ( R0.L , 37 );
	R0 += -7;
	DBGA ( R0.L , 30 );
	pass
