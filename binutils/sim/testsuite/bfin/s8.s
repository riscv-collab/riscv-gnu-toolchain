//  Test  rl4 = VMAX r5  A0<<1;
//  Test  rl4 = VMAX r5  A0>>1;
# mach: bfin

.include "testutils.inc"
	start


// max value in high half, hence bit into A0 is one
	A0 = 0;
	R1.L = 0x2;	// max in r1 is 3
	R1.H = 0x3;

	R6.L = VIT_MAX( R1 ) (ASL);

	DBGA ( R6.L , 0x0003 );
	R7 = A0.w;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0000 );

// max value in low half, hence bit into A0 is zero
	R0.L = 0x8000;
	R0.H = 0x8000;
	A0.w = R0;
	R1.L = 0x8001;	// max in r1 is 8001
	R1.H = 0x7f00;

	R6.L = VIT_MAX( R1 ) (ASL);

	DBGA ( R6.L , 0x8001 );
	R7 = A0.w;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0001 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0001 );

// max value in high half, hence bit into A0 is one
	R0.L = 0x8000;
	R0.H = 0x0000;
	A0.w = R0;
	R1.L = 0x7fff;	// max in r1 is 8001
	R1.H = 0x8001;

	R6.L = VIT_MAX( R1 ) (ASR);

	DBGA ( R6.L , 0x8001 );
	R7 = A0.w;
	DBGA ( R7.L , 0x4000 );
	DBGA ( R7.H , 0x8000 );
	R7.L = A0.x;
	DBGA ( R7.L , 0x0000 );

	pass
