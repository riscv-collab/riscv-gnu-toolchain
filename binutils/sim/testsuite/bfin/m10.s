# mach: bfin

//  Test extraction from accumulators:
//  ROUND/TRUNCATE in UNSIGNED FRACTIONAL mode
//  test ops: "+="

.include "testutils.inc"
	start


// load r0=0xfffef000
// load r1=0xfffff000
// load r2=0x00008000
// load r3=0x00018000
// load r4=0x0000007f
	loadsym P0, data0
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// round
// 0x00fffef000 -> 0xffff
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1, R5.L = A0 (FU);
	DBGA ( R5.L , 0xffff );
	DBGA ( R5.H , 0xffff );

// truncate
// 0x00fffef00 -> 0xfffe
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1, R5.L = A0 (TFU);
	DBGA ( R5.L , 0xfffe );
	DBGA ( R5.H , 0xfffe );

// round
// 0x00fffff000 -> 0xffff
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R5.H = A1, R5.L = A0 (FU);
	DBGA ( R5.L , 0xffff );
	DBGA ( R5.H , 0xffff );

	pass

	.data;
data0:
	.dw 0xf000
	.dw 0xfffe
	.dw 0xf000
	.dw 0xffff
	.dw 0x8000
	.dw 0x0000
	.dw 0x8000
	.dw 0x0001
	.dw 0x007f
	.dw 0x0000
