//  REG-BASED dual 16b SHIFT test program.
//  Test  r4 = ASHIFT/ASHIFT (r2 by rl1);
//  Test  r4 = ASHIFT/ASHIFT (r2 by rl1) S;
//  Test  r4 = LSHIFT/LSHIFT (r2 by rl1);
# mach: bfin

.include "testutils.inc"
	start


// arithmetic
//  left by largest positive magnitude of 15 (0xf)
// 8001 -> 8000
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8001;
	R0.H = 0x0100;
	R1.L = 15;
	R6 = ASHIFT R0 BY R1.L (V);
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by largest positive magnitude of 15 (0xf) with saturation
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8001;
	R0.H = 0x0100;
	R1.L = 15;
	R6 = ASHIFT R0 BY R1.L (V , S);
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by 1
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8001;
	R0.H = 0x0100;
	R1.L = 1;
	R6 = ASHIFT R0 BY R1.L (V);
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0200 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by 1 saturating
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8001;
	R0.H = 0x0100;
	R1.L = 1;
	R6 = ASHIFT R0 BY R1.L (V , S);
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x0200 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by 15 saturating
	R7 = 0;
	ASTAT = R7;
	R0.L = 0xfff0;
	R0.H = 0x0000;
	R1.L = 15;
	R6 = ASHIFT R0 BY R1.L (V , S);
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  right by 15
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8000;
	R0.H = 0x0100;
	R1.L = -15;
	R6 = ASHIFT R0 BY R1.L (V);
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  right by 15 (sat has no effect)
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8000;
	R0.H = 0x0100;
	R1.L = -15;
	R6 = ASHIFT R0 BY R1.L (V , S);
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//  right by 15
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x8000;
	R0.H = 0x0100;
	R1.L = -15;
	R6 = LSHIFT R0 BY R1.L (V);
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	pass
