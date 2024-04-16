//  Test ALU  SEARCH instruction
# mach: bfin

.include "testutils.inc"
	start


	init_r_regs 0;
	ASTAT = R0;

	R0 = 4;
	R1 = 5;
	A1 = A0 = 0;

	R2.L = 0x0001;
	R2.H = 0xffff;

	loadsym P0, foo;

	( R1 , R0 ) = SEARCH R2 (GT);

	// R0 should be the pointer
	R7 = P0;
	CC = R0 == R7;
	if !CC JUMP _fail;

	_DBG R1;	// does not change
	DBGA ( R1.H , 0 );	DBGA ( R1.L , 0x5 );

	_DBG A0;	// changes
	R0 = A0.w;
	DBGA ( R0.H , 0 );	DBGA ( R0.L , 0x1 );

	_DBG A1;	// does not change
	R0 = A1.w;
	DBGA ( R0.H , 0 );	DBGA ( R0.L , 0 );

	R0 = 4;
	R1 = 5;
	A1 = A0 = 0;

	R2.L = 0x0000;
	R2.H = 0xffff;

	loadsym p0, foo;

	( R1 , R0 ) = SEARCH R2 (LT);

	_DBG R0;	// no change
	DBGA ( R0.H , 0 );	DBGA ( R0.L , 4 );

	_DBG R1;	// change
	R7 = P0;
	CC = R1 == R7;
	if !CC JUMP _fail;

	_DBG A0;
	R0 = A0.w;
	DBGA ( R0.H , 0 );	DBGA ( R0.L , 0 );

	_DBG A1;
	R0 = A1.w;
	DBGA ( R0.H , 0xffff );	DBGA ( R0.L , 0xffff );

	pass

_fail:
	fail;

	.data
foo:
	.space (0x100)
