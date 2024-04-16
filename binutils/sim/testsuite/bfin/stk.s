# mach: bfin

.include "testutils.inc"
	start


// load up some registers.
// setup up a global pointer table and load some state.
// save the machine state and clear some of the values.
// then restore and assert some of the values to ensure that
// we maintain consitent machine state.

	R0 = 1;
	R1 = 2;
	R2 = 3;
	R3 = -7;
	R4 = 4;
	R5 = 5;
	R6 = 6;
	R7 = 7;

	loadsym P0, a;
	_DBG P0;
	SP = P0;
	FP = P0;
	P1 = [ P0 ++ ];
	P2 = [ P0 ++ ];
	P0 += 4;
	P4 = [ P0 ++ ];
	P5 = [ P0 ++ ];
	[ -- SP ] = ( R7:0, P5:0 );
	_DBG SP;
	_DBG FP;
	R0 = R0 ^ R0;
	R1 = R1 ^ R1;
	R2 = R2 ^ R2;
	R4 = R4 ^ R4;
	R5 = R5 ^ R5;
	R6 = R6 ^ R6;
	R7 = R7 ^ R7;
	( R7:0, P5:0 ) = [ SP ++ ];
	DBGA ( R0.L , 1 );
	DBGA ( R2.L , 3 );
	DBGA ( R7.L , 7 );
	R0 = SP;
	loadsym R1, a;
	CC = R0 == R1;
	IF !CC JUMP abrt;
	R0 = FP;
	CC = R0 == R1;
	CC = R0 == R1;
	IF !CC JUMP abrt;

	pass
abrt:
	fail

	.data
_gptab:
	.dw 0x200
	.dw 0x000
	.dw 0x300
	.dw 0x400
	.dw 0x500
	.dw 0x600

	.space (0x100)
a:
	.dw 1
	.dw 2
	.dw 3
	.dw 4
	.dw 5
	.dw 6
	.dw 7
	.dw 8
	.dw 9
	.dw 0xa
