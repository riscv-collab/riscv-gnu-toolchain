//  reg-based SHIFT test program.
//  Test  r4 = ASHIFT (r2 by rl3);
//  Test  r4 = LSHIFT (r2 by rl3);
# mach: bfin

.include "testutils.inc"
	start


	R0.L = 0x0001;
	R0.H = 0x8000;

// arithmetic
//  left by  31
// 8000 0001 -> 8000 0000
	R7 = 0;
	ASTAT = R7;
	R3.L = 31;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by  32
// 8000 0001 -> 8000 0000
	R7 = 0;
	ASTAT = R7;
	R3.L = 32;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by  40
// 8000 0001 -> 8000 0000
	R7 = 0;
	ASTAT = R7;
	R3.L = 40;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0xFF80 );
	DBGA ( R6.H , 0xFFFF );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by  -32
// 8000 0001 -> 8000 0000
	R7 = 0;
	ASTAT = R7;
	R3.L = -32;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by  63 (off scale)
// 8000 0001 -> 0000 0000
	R7 = 0;
	ASTAT = R7;
	R0.L = 1;
	R0.H = 0;
	R3.L = 63;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by  255 looks like -1 (mask 7 bits)
// 8000 0001 -> 0000 0000
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x0100;
	R0.H = 0;
	R3.L = 255;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0080 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by 1
// 8000 0001 -> 0000 0002
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = 1;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  right by 1
// 8000 0001 -> 0000 0002
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = -1;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xc000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  right by  -31
// 8000 0001 -> ffff ffff
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = -31;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//  left by largest positive magnitude of 31 (0x1f)
// 8000 0001 -> 8000 0000
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = 31;
	R3.H = 0;
	R6 = ASHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//  left by 1
// 8000 0001 -> 0000 0002
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = 1;
	R3.H = 0;
	R6 = LSHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//  right by 1
// 8000 0001 -> 4000 0000
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = -1;
	R3.H = 0;
	R6 = LSHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//  right by largest negative magnitude of -31
// 8000 0001 -> 0000 0001
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = -31;
	R3.H = 0;
	R6 = LSHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//  right by -32
// 8000 0001 -> 0000 0001
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = -32;
	R3.H = 0;
	R6 = LSHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic
//   by +40
// 8000 0001 -> 0000 0001
	R0.L = 0x0001;
	R0.H = 0x8000;
	R3.L = 40;
	R3.H = 0;
	R6 = LSHIFT R0 BY R3.L;
	DBGA ( R6.L , 0x0080 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// rot
//  left  by 1
// 8000 0001 -> 0000 0002 cc=1
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY 1;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;	DBGA ( R7.L , 0x0001 );

// rot
//  right by -1
// 8000 0001 -> 4000 0000 cc=1
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY -1;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	R7 = CC;	DBGA ( R7.L , 0x0001 );

// rot
//  right by largest positive magnitude of 31
// 8000 0001 -> a000 0000 cc=0
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY 31;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xa000 );
	R7 = CC;	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest positive magnitude of 31 with cc=1
// 8000 0001 cc=1 -> a000 0000 cc=0
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY 31;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xe000 );
	R7 = CC;	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest negative magnitude of -31
// 8000 0001 -> 0000 0005 cc=0
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY -31;
	DBGA ( R6.L , 0x0005 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest negative magnitude of -31 with cc=1
// 8000 0001 cc=1 -> 0000 0007 cc=0
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY -31;
	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;	DBGA ( R7.L , 0x0000 );

// rot
//  left by 7
// 8000 0001 cc=1 -> 0000 00e0 cc=0
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY 7;
	DBGA ( R6.L , 0x00e0 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;	DBGA ( R7.L , 0x0000 );

// rot by zero
// 8000 0001 -> 8000 000
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY 0;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x8000 );
	R7 = CC;	DBGA ( R7.L , 0x0001 );

//  0 by 1
	R7 = 0;
	R0 = 0;
	ASTAT = R7;
	R6 = R0 << 1;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	pass
