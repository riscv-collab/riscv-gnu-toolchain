# mach: bfin
.include "testutils.inc"
	start

	P0 = 10;

	LSETUP ( xxx , yyy ) LC0 = P0;
xxx:
	R1 += 1;
	CC = R1 == 3;
yyy:
	IF CC JUMP zzz;
	R3 = 7;
zzz:
	DBGA ( R1.L , 3 );
	pass
