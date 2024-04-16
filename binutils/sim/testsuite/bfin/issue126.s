# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;
	P1 = R0;
	P2 = R0;

	R0 = R0;
	P1 = ( P1 + P0 ) << 2;
	P2 = ( P2 + P0 ) << 1;

	_DBG ASTAT;
	R5 = ASTAT;
	DBGA ( R5.H , 0 );	DBGA ( R5.L , 0 );

	pass
