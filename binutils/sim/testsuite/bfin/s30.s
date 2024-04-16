//  Test signbits40
# mach: bfin

.include "testutils.inc"
	start


// positive value in accum, smaller than 1.0
	A1 = A0 = 0;
	R0.L = 0xffff;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x7fff );	DBGA ( R4.L , 0x8000 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0000 );

// neg value in accum, larger than -1.0
	A1 = A0 = 0;
	R0.L = 0x0000;
	R0.H = 0xffff;
	A0.w = R0;
	R0.L = 0x00ff;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x8000 );	DBGA ( R4.L , 0x0000 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xffff );

// positive value in accum, larger than 1.0
	A1 = A0 = 0;
	R0.L = 0xffff;
	R0.H = 0xffff;
	A0.w = R0;
	R0.L = 0x000f;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x7fff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0000 );

// negative value in accum, smaller than -1.0
	A1 = A0 = 0;
	R0.L = 0x0000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0080;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x8000 );	DBGA ( R4.L , 0x0000 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xffff );

// no normalization
	A1 = A0 = 0;
	R0.L = 0xfffa;
	R0.H = 0x7fff;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x7fff );	DBGA ( R4.L , 0xfffa );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0000 );

// no normalization (-1.0)
	A1 = A0 = 0;
	R0.L = 0x0000;
	R0.H = 0x8000;
	A0.w = R0;
	R0.L = 0x00ff;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x8000 );	DBGA ( R4.L , 0x0000 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xffff );

// norm by 1
	A1 = A0 = 0;
	R0.L = 0x0000;
	R0.H = 0x8000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x4000 );	DBGA ( R4.L , 0x0000 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0000 );

// norm by 1
	A1 = A0 = 0;
	R0.L = 0x0000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x00ff;
	A0.x = R0;

	R5.L = SIGNBITS A0;
	_DBG R5;
	A0 = ASHIFT A0 BY R5.L;
	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x8000 );	DBGA ( R4.L , 0x0000 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xffff );

	pass
