//  Test extraction from accumulators:
//  ROUND/TRUNCATE in SIGNED FRACTIONAL mode
//  test ops: "+="
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x7ffef000
// load r1=0x7ffff000
// load r2=0x00008000
// load r3=0x00018000
// load r4=0x0000007f
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// round
// 0x007ffef00 -> 0x7fff
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1, R5.L = A0;
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );

// round with ovflw
// 0x007ffff00 -> 0x7fff
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R5.H = A1, R5.L = A0;
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );

// trunc
// 0x007ffef00 -> 0x7ffe
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1, R5.L = A0 (T);
	DBGA ( R5.L , 0x7ffe );
	DBGA ( R5.H , 0x7ffe );

// round with ovflw
// 0x7f7ffff00 -> 0x7fff
	A1 = A0 = 0;
	A1.w = R1;
	A1.x = R4.L;
	A0.w = R1;
	A0.x = R4.L;
	R5.H = A1, R5.L = A0;
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );

// round, nearest even is zero
// 0x0000008000 -> 0x0000
	A1 = A0 = 0;
	A1.w = R2;
	A0.w = R2;
	R5.H = A1, R5.L = A0;
	DBGA ( R5.L , 0x0 );
	DBGA ( R5.H , 0x0 );

// round, nearest even is 2
// 0x00000018000 -> 0x0002
	A1 = A0 = 0;
	A1.w = R3;
	A0.w = R3;
	R5.H = A1, R5.L = A0;
	DBGA ( R5.L , 0x2 );
	DBGA ( R5.H , 0x2 );

	pass

	.data
data0:
	.dw 0xf000
	.dw 0x7ffe
	.dw 0xf000
	.dw 0x7ffe
	.dw 0x8000
	.dw 0x0000
	.dw 0x8000
	.dw 0x0001
	.dw 0x007f
	.dw 0x0000
