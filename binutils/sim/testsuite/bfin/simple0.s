# mach: bfin

.include "testutils.inc"
	start


	R0 = 5;
	R0 += -1;
	DBGA ( R0.L , 4 );
	pass
