# mach: bfin

.include "testutils.inc"
	start

	R0 = 10;
	P0 = R0;
	LSETUP ( ls0 , ls0 ) LC0 = P0;
ls0:
	R0 += -1;
	DBGA ( R0.L , 0 );
	pass
