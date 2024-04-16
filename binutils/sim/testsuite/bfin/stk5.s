# mach: bfin

.include "testutils.inc"
	start

	SP += -12;
	FP = SP;
	CALL _foo;

	pass


_printf:
	LINK 0;
	[ -- SP ] = ( R7:7, P5:4 );
	R5 = [ FP + 8 ];
	DBGA ( R5.L , 0x1234 );
	R5 = [ FP + 12 ];
	DBGA ( R5.L , 0xdead );
	( R7:7, P5:4 ) = [ SP ++ ];
	UNLINK;
	RTS;

_foo:
	LINK 0;
	R5 = 0xdead (Z);
	[ -- SP ] = R5;
	R5 = 0x1234 (X);
	[ -- SP ] = R5;
	CALL _printf;
	P5 = 8;
	SP = SP + P5;
	UNLINK;
	RTS;
