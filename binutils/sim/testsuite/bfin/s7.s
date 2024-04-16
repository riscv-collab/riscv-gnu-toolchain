//  Test  r4 = VMAX/VMAX (r5,r1)  A0>>2;
# mach: bfin

.include "testutils.inc"
	start


// Both max values are in high half, hence both bits
// into A0 are 1
	A0 = 0;
	R1.L = 0x2;	// max in r1 is 3
	R1.H = 0x3;

	R0.L = 0x6;	// max in r0 is 7
	R0.H = 0x7;

	R6 = VIT_MAX( R1 , R0 ) (ASR);

	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x0003 );
	R7 = A0.w;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0xc000 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0000 );

// max value in r1 is in low, so second bit into A0 is zero
	A0 = 0;
	R1.L = 0x3;	// max in r1 is 3
	R1.H = 0x2;

	R0.L = 0x6;	// max in r0 is 7
	R0.H = 0x7;

	R6 = VIT_MAX( R1 , R0 ) (ASR);

	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x0003 );
	R7 = A0.w;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x4000 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0000 );

// both max values in low, so both bits into A0 are zero
	R0.L = 0x8000;
	R0.H = 0x0;
	A0.w = R0;
	R1.L = 0x3;	// max in r1 is 3
	R1.H = 0x2;

	R0.L = 0x7;	// max in r0 is 7
	R0.H = 0x6;

	R6 = VIT_MAX( R1 , R0 ) (ASR);

	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x0003 );
	R7 = A0.w;
	DBGA ( R7.L , 0x2000 );
	DBGA ( R7.H , 0x0000 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0000 );

// Test for correct max when one value overflows
	A0 = 0;
	R1.L = 0x7fff;	// max in r1 is 0x8001 (overflowed)
	R1.H = 0x8001;

	R0.L = 0x6;	// max in r0 is 7
	R0.H = 0x7;

	R6 = VIT_MAX( R1 , R0 ) (ASR);

	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x8001 );
	R7 = A0.w;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0xc000 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0000 );

	pass
