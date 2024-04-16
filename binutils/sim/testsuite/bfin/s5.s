//  Test  r4 = ROT    (r2 by r3);
# mach: bfin

.include "testutils.inc"
	start


	R0.L = 0x0001;
	R0.H = 0x8000;

// rot
//  left  by 1
// 8000 0001 -> 0000 0002 cc=1
	R7 = 0;
	CC = R7;
	R1 = 1;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
//  right by -1
// 8000 0001 -> 4000 0000 cc=1
	R7 = 0;
	CC = R7;
	R1.L = 0xffff;	// check alternate mechanism for immediates
	R1.H = 0xffff;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
//  right by largest positive magnitude of 31
// 8000 0001 -> a000 0000 cc=0
	R7 = 0;
	CC = R7;
	R1 = 31;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xa000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest positive magnitude of 31 with cc=1
// 8000 0001 cc=1 -> a000 0000 cc=0
	R7 = 1;
	CC = R7;
	R1 = 31;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xe000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest negative magnitude of -31
// 8000 0001 -> 0000 0005 cc=0
	R7 = 0;
	CC = R7;
	R1 = -31;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0005 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest negative magnitude of -31 with cc=1
// 8000 0001 cc=1 -> 0000 0007 cc=0
	R7 = 1;
	CC = R7;
	R1 = -31;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  left by 7
// 8000 0001 cc=1 -> 0000 00e0 cc=0
	R7 = 1;
	CC = R7;
	R1 = 7;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x00e0 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot by zero
// 8000 0001 -> 8000 0000
	R7 = 1;
	CC = R7;
	R1 = 0;
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x8000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot by 0b1100 0001 is the same as by 1 (mask 6 bits)
// 8000 0001 -> 0000 0002 cc=1
	R7 = 0;
	CC = R7;
	R1 = 0xc1 (X);
	R6 = ROT R0 BY R1.L;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

	pass
