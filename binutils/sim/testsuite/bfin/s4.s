//  Immediate SHIFT test program.
//  Test  r4 = ASHIFT (r2 by 10);
//  Test  r4 = LSHIFT (r2 by 10);
//  Test  r4 = ROT    (r2 by 10);
# mach: bfin

.include "testutils.inc"
	start


	init_r_regs 0;
	ASTAT = R0;

// load r0=0x80000001
// load r1=0x00000000
// load r2=0x00000000
// load r3=0x00000000
// load r4=0x00000000
// load r5=0x00000000
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];
	R5 = [ P0 ++ ];

// arithmetic
//  left by largest positive magnitude of 31 (0x1f)
// 8000 0001 -> 8000 0000
	R7 = 0;
	ASTAT = R7;
	R6 = R0 << 31;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  left by 1
// 8000 0001 -> 0000 0002
	R6 = R0 << 1;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  right by 1
// 8000 0001 -> c000 0000
	R7 = 0;
	ASTAT = R7;
	R6 = R0 >>> 1;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xc000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// arithmetic
//  right by largest negative magnitude of -31
// 8000 0001 -> ffff ffff
	R6 = R0 >>> 31;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );

// logic
//  left by largest positive magnitude of 31 (0x1f)
// 8000 0001 -> 8000 0000
	R6 = R0 << 31;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );

// logic
//  left by 1
// 8000 0001 -> 0000 0002
	R6 = R0 << 1;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );

// logic
//  right by 1
// 8000 0001 -> 4000 0000
	R6 = R0 >> 1;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );

// logic
//  right by largest negative magnitude of -31
// 8000 0001 -> 0000 0001
	R6 = R0 >> 31;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x0000 );

// rot
//  left  by 1
// 8000 0001 -> 0000 0002 cc=1
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY 1;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
//  right by -1
// 8000 0001 -> 4000 0000 cc=1
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY -1;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
//  right by largest positive magnitude of 31
// 8000 0001 -> a000 0000 cc=0
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY 31;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xa000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest positive magnitude of 31 with cc=1
// 8000 0001 cc=1 -> a000 0000 cc=0
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY 31;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xe000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest negative magnitude of -31
// 8000 0001 -> 0000 0005 cc=0
	R7 = 0;
	CC = R7;
	R6 = ROT R0 BY -31;
	DBGA ( R6.L , 0x0005 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  right by largest negative magnitude of -31 with cc=1
// 8000 0001 cc=1 -> 0000 0007 cc=0
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY -31;
	DBGA ( R6.L , 0x0007 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
//  left by 7
// 8000 0001 cc=1 -> 0000 00e0 cc=0
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY 7;
	DBGA ( R6.L , 0x00e0 );
	DBGA ( R6.H , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot by zero
// 8000 0001 -> 8000 000
	R7 = 1;
	CC = R7;
	R6 = ROT R0 BY 0;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x8000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

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

	.data
data0:
	.dw 0x0001
	.dw 0x8000
	.dd 0x0000
	.dd 0x0
	.dd 0x0
	.dd 0x0
	.dd 0x0
	.dd 0x0
