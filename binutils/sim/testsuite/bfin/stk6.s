// setup a dummy stack and put values in memory 0,1,2,3...n
// then restore registers with pop instruction.
# mach: bfin

.include "testutils.inc"
	start

	SP += -12;

	P1 = SP;
	R1 = 0;
	P5.L = 0xdead;
	SP += -((8+5)*4);	// lets move the stack pointer and include the current location. i.e. 5
	P4 = (8+6);		// 8 data registers and 6 pointer registers are being stored.
	LSETUP ( ls0 , le0 ) LC0 = P4;
ls0:
	R1 += 1;
le0:
	[ P1-- ] = R1;

	( R7:0, P5:0 ) = [ SP ++ ];

	DBGA ( R0.L , 1 );
	DBGA ( R1.L , 2 );
	DBGA ( R2.L , 3 );
	DBGA ( R3.L , 4 );
	DBGA ( R4.L , 5 );
	DBGA ( R5.L , 6 );
	DBGA ( R6.L , 7 );
	DBGA ( R7.L , 8 );
	R0 = P0;	DBGA ( R0.L , 9 );
	R0 = P1;	DBGA ( R0.L , 10 );
	R0 = P2;	DBGA ( R0.L , 11 );
	R0 = P3;	DBGA ( R0.L , 12 );
	R0 = P4;	DBGA ( R0.L , 13 );
	R0 = P5;	DBGA ( R0.L , 14 );
	R0 = 1;

	[ -- SP ] = ( R7:0, P5:0 );
	( R7:0, P5:0 ) = [ SP ++ ];

	DBGA ( R0.L , 1 );
	DBGA ( R1.L , 2 );
	DBGA ( R2.L , 3 );
	DBGA ( R3.L , 4 );
	DBGA ( R4.L , 5 );
	DBGA ( R5.L , 6 );
	DBGA ( R6.L , 7 );
	DBGA ( R7.L , 8 );
	R0 = P0;	DBGA ( R0.L , 9 );
	R0 = P1;	DBGA ( R0.L , 10 );
	R0 = P2;	DBGA ( R0.L , 11 );
	R0 = P3;	DBGA ( R0.L , 12 );
	R0 = P4;	DBGA ( R0.L , 13 );
	R0 = P5;	DBGA ( R0.L , 14 );
	R0 = 1;

	pass
