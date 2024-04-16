// Test various moves to single register half
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x7fffffff
// load r1=0x00ffffff
// load r2=0xf0000000
// load r3=0x0000007f
// load r4=0x00000080
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// extract  only to high half
	R5 = 0;
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1;
	DBGA ( R5.L , 0x0000 );
	DBGA ( R5.H , 0x7fff );

// extract only to low half
	R5 = 0;
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.L = A0;
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x0000 );

// extract  only to high half
	R5 = 0;
	A1 = A0 = 0;
	R5.H = ( A1 += R0.H * R0.H ), A0 += R0.H * R0.H;
	DBGA ( R5.L , 0x0000 );
	DBGA ( R5.H , 0x7ffe );

// extract  only to low half
	R5 = 0;
	A1 = A0 = 0;
	A1 += R0.H * R0.H, R5.L = ( A0 += R0.H * R0.H );
	DBGA ( R5.L , 0x7ffe );
	DBGA ( R5.H , 0x0000 );

	pass

	.data
data0:
	.dw 0xffff
	.dw 0x7fff
	.dw 0xffff
	.dw 0x00ff
	.dw 0x0000
	.dw 0xf000
	.dw 0x007f
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
