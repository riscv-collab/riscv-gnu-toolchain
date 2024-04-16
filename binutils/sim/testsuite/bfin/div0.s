# mach: bfin

.include "testutils.inc"
	start

	R0 = 70 (X);
	R1 = 5;

	P2 = 16;
	DIVS ( R0 , R1 );
	LSETUP ( s0 , s0 ) LC0 = P2;
s0:
	DIVQ ( R0 , R1 );

	DBGA ( R0.L , 14 );

	R0 = 3272 (X);
	R1 = 55;

	DIVS ( R0 , R1 );
	LSETUP ( s1 , s1 ) LC0 = P2;
s1:
	DIVQ ( R0 , R1 );

	DBGA ( R0.L , 59 );

	R0 = 32767 (X);
	R1 = 55;
	DIVS ( R0 , R1 );

	LSETUP ( s2 , s2 ) LC0 = P2;
s2:
	DIVQ ( R0 , R1 );

	DBGA ( R0.L , 595 );

	pass
