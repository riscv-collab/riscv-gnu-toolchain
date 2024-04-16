//  Test extraction from accumulators:
//  SCALE in SIGNED INTEGER mode
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x00000fff
// load r1=0x00007fff
// load r2=0xffffffff
// load r3=0xffff0fff
// load r4=0x000000ff
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// SCALE
//  0x0000000fff -> SCALE 0x1ffe
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1, R5.L = A0 (ISS2);
	DBGA ( R5.L , 0x1ffe );
	DBGA ( R5.H , 0x1ffe );

// SCALE
//  0x0000007fff -> SCALE 0x7fff
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R5.H = A1, R5.L = A0 (ISS2);
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );

// SCALE
//  0xffffffffff -> SCALE 0xfffe
	A1 = A0 = 0;
	A1.w = R2;
	A0.w = R2;
	A1.x = R4.L;
	A0.x = R4.L;
	R5.H = A1, R5.L = A0 (ISS2);
	DBGA ( R5.L , 0xfffe );
	DBGA ( R5.H , 0xfffe );

// SCALE
//  0xffffff0fff -> SCALE 0x8000
	A1 = A0 = 0;
	A1.w = R3;
	A0.w = R3;
	A1.x = R4.L;
	A0.x = R4.L;
	R5.H = A1, R5.L = A0 (ISS2);
	DBGA ( R5.L , 0x8000 );
	DBGA ( R5.H , 0x8000 );

	pass

	.data
data0:
	.dw 0x0fff
	.dw 0x0000
	.dw 0x7fff
	.dw 0x0000
	.dw 0xffff
	.dw 0xffff
	.dw 0x0fff
	.dw 0xffff
	.dw 0x00ff
	.dw 0x0000
