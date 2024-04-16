// Test various moves to single register
# mach: bfin


.include "testutils.inc"
	start


// load r0=0x7fffffff
// load r1=0x00ffffff
// load r2=0xf0000000
// load r3=0x0000007f
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];

// extract only to high register
	R5 = 0;
	R4 = 0;
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5 = A1;
	DBGA ( R4.L , 0x0000 );
	DBGA ( R4.H , 0x0000 );
	DBGA ( R5.L , 0xffff );
	DBGA ( R5.H , 0x7fff );

// extract only to low register
	R5 = 0;
	R4 = 0;
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R4 = A0;
	DBGA ( R4.L , 0xffff );
	DBGA ( R4.H , 0x7fff );
	DBGA ( R5.L , 0x0000 );
	DBGA ( R5.H , 0x0000 );

// extract  only to high reg
	R5 = 0;
	R4 = 0;
	A1 = A0 = 0;
	R5 = ( A1 += R0.H * R0.H ), A0 += R0.H * R0.H;
	DBGA ( R4.L , 0x0000 );
	DBGA ( R4.H , 0x0000 );
	DBGA ( R5.L , 0x0002 );
	DBGA ( R5.H , 0x7ffe );

// extract  only to low reg
	R5 = 0;
	R4 = 0;
	A1 = A0 = 0;
	A1 += R0.H * R0.H, R4 = ( A0 += R0.H * R0.H );
	DBGA ( R4.L , 0x0002 );
	DBGA ( R4.H , 0x7ffe );
	DBGA ( R5.L , 0x0000 );
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
