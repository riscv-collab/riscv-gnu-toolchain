//  Test extraction from accumulators:
//  SCALE in SIGNED FRACTIONAL mode
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x3fff0000
// load r1=0x0fffc000
// load r2=0x7ff00000
// load r3=0x80100000
// load r4=0x000000ff
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// SCALE
//  0x003fff0000 -> SCALE 0x7ffe
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R5.H = A1, R5.L = A0 (S2RND);
	DBGA ( R5.L , 0x7ffe );
	DBGA ( R5.H , 0x7ffe );

// SCALE
//  0x000fffc000 -> SCALE 0x2000
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R5.H = A1, R5.L = A0 (S2RND);
	DBGA ( R5.L , 0x2000 );
	DBGA ( R5.H , 0x2000 );

// SCALE
//  0x007ff00000 -> SCALE 0x7fff
	A1 = A0 = 0;
	A1.w = R2;
	A0.w = R2;
	R5.H = A1, R5.L = A0 (S2RND);
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );

// SCALE
//  0xff80100000 -> SCALE 0x8000
	A1 = A0 = 0;
	A1.w = R3;
	A0.w = R3;
	A1.x = R4.L;
	A0.x = R4.L;
	R5.H = A1, R5.L = A0 (S2RND);
	DBGA ( R5.L , 0x8000 );
	DBGA ( R5.H , 0x8000 );

	pass

	.data;
data0:
	.dw 0x0000
	.dw 0x3fff
	.dw 0xc000
	.dw 0x0fff
	.dw 0x0000
	.dw 0x7ff0
	.dw 0x0000
	.dw 0x8010
	.dw 0x00ff
	.dw 0x0000
