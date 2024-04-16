//  Test extraction from accumulators:
//  SIGNED FRACTIONAL and SIGNED INT mode into register PAIR with SCALE
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x0ffffff0
// load r1=0x7ffffff0
// load r2=0x0fffffff
// load r3=0x80100000
// load r4=0x000000ff
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// extract
// 0x000ffffff0 -> 0x1ffffffe0
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R7 = A1,  R6 = A0  (S2RND);
	DBGA ( R7.L , 0xffe0 );
	DBGA ( R7.H , 0x1fff );
	DBGA ( R6.L , 0xffe0 );
	DBGA ( R6.H , 0x1fff );

// extract (saturate)
// 0x007ffffff0 -> 0x7ffffffff
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R7 = A1,  R6 = A0  (S2RND);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0x7fff );

// extract (saturate negative)
// 0xff0ffffff0 -> 0x80000000
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	A1.x = R4.L;
	A0.x = R4.L;
	R7 = A1,  R6 = A0  (S2RND);
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x8000 );
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );

// extract int
// 0x000ffffff0 -> 0x1ffffffe0
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R7 = A1,  R6 = A0  (ISS2);
	DBGA ( R7.L , 0xffe0 );
	DBGA ( R7.H , 0x1fff );
	DBGA ( R6.L , 0xffe0 );
	DBGA ( R6.H , 0x1fff );

	pass

	.data
data0:
	.dw 0xfff0
	.dw 0x0fff
	.dw 0xfff0
	.dw 0x7fff
	.dw 0xffff
	.dw 0x0fff
	.dw 0x0000
	.dw 0x8010
	.dw 0x00ff
	.dw 0x0000
