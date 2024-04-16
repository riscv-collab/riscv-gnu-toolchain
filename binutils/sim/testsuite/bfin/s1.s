# mach: bfin

.include "testutils.inc"
	start

	R0 = 1;
	R1 = 2;
	R2 = 3;
	R4 = 4;
	P1 = R1;
	LSETUP ( ls0 , ls0 ) LC0 = P1;
	R5 = 5;
	R6 = 6;
	R7 = 7;

ls0: R1 += 1;

	DBGA ( R1.L , 4 );
	P1 = R1;
	LSETUP ( ls1 , ls1 ) LC1 = P1;
ls1: R1 += 1;

	DBGA ( R1.L , 8 );

	pass
