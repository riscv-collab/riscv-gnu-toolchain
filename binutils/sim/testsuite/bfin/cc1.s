# mach: bfin

.include "testutils.inc"
	start


	R0 = 0x1234 (X);
	CC = BITTST ( R0 , 2 );
	IF !CC JUMP s$0;
	R0 += 1;
s$0:
	nop;
	DBGA ( R0.L , 0x1235 );
	CC = BITTST ( R0 , 1 );
	IF !CC JUMP s$1;
	R0 += 1;
s$1:
	nop;
	DBGA ( R0.L , 0x1235 );
	CC = BITTST ( R0 , 12 );
	IF !CC JUMP s$3;
	R0 = - R0;
s$3:
	nop;
	DBGA ( R0.L , 0xedcb );
	pass
