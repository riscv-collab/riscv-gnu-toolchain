//  SHIFT test program.
//  Test A0 = ASHIFT  (A0 by r3);
# mach: bfin

.include "testutils.inc"
	start

// load r0=0x0000001f
// load r1=0x00000020
// load r2=0x00000000
// load r3=0x00000000
// load r4=0x00000001
// load r5=0x00000080
	loadsym P0, data0;
	P1 = P0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];
	R5 = [ P0 ++ ];

//  left by largest positive magnitude of 31 (0x1f)
// A0: 80 0000 0001 -> 80 0000 0000
	R7 = 0;
	ASTAT = R7;
	A0.w = R4;
	A0.x = R5.L;
	A0 = ASHIFT A0 BY R0.L;
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	DBGA ( R7.L , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

//  left by largest positive magnitude + 1 = 32 (0x20), which is -32
// A0: 80 0000 0001 ->
	R7 = 0;
	ASTAT = R7;
	A0.w = R4;
	A0.x = R5.L;
	A0 = ASHIFT A0 BY R1.L;
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0xff80 );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

//  by -1
// A0: 80 0000 0001 -> c0 0000 0000
	A0.w = R4;
	A0.x = R5.L;

	R3.L = 0x00ff;

	A0 = ASHIFT A0 BY R3.L;
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xffc0 );

	pass

	.data
data0:
	.dw 0x001f
	.dw 0x0000
	.dw 0x0020
	.dw 0x0000
	.dw 0x0059
	.dw 0x0000
	.dw 0x005a
	.dw 0x0000
	.dw 0x0001
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
