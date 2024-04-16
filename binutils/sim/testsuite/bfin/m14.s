//  Test extraction from accumulators:
//  UNSIGNED FRACTIONAL and SIGNED INT mode into register PAIR
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x7ffffff0
// load r1=0xfffffff0
// load r2=0x0fffffff
// load r3=0x00000001
// load r4=0x000000ff
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// extract
// 0x00fffffff0 -> 0xffffffff0
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R7 = A1,  R6 = A0  (FU);
	DBGA ( R7.L , 0xfff0 );
	DBGA ( R7.H , 0xffff );
	DBGA ( R6.L , 0xfff0 );
	DBGA ( R6.H , 0xffff );

// extract with saturation
// 0x01fffffff0 -> 0xfffffffff
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	A1.x = R3.L;
	A0.x = R3.L;
	R7 = A1,  R6 = A0  (FU);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0xffff );
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );

// extract with saturation
// 0xfffffffff0 -> 0xfffffffff
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	A1.x = R4.L;
	A0.x = R4.L;
	R7 = A1,  R6 = A0  (FU);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0xffff );
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );

// extract unsigned
// 0x00fffffff0 -> 0xffffffff0
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R7 = A1,  R6 = A0  (FU);
	DBGA ( R7.L , 0xfff0 );
	DBGA ( R7.H , 0xffff );
	DBGA ( R6.L , 0xfff0 );
	DBGA ( R6.H , 0xffff );

	pass

	.data
data0:
	.dw 0xfff0
	.dw 0x7fff
	.dw 0xfff0
	.dw 0xffff
	.dw 0xffff
	.dw 0x0fff
	.dw 0x0001
	.dw 0x0000
	.dw 0x00ff
	.dw 0x0000
