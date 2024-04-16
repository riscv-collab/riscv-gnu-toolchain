//  Test result extraction of mac instructions.
//  Test basic edge values
//  UNSIGNED FRACTIONAL mode into SINGLE destination register
//  test ops: "+="
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x80000001
// load r1=0x80007fff
// load r2=0xf000ffff
// load r3=0x0000007f
// load r4=0x00000080
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// extraction with no saturation (truncate)
// 0x8000 * 0x7fff = 0x003fff8000 -> 0x3fff
	A1 = A0 = 0;
	R5.H = (A1 += R0.H * R1.L), R5.L = (A0 += R0.H * R1.L) (TFU);
	DBGA ( R5.L , 0x3fff );
	DBGA ( R5.H , 0x3fff );

// extraction with no saturation (round)
// 0x8000 * 0x7fff = 0x003fff8000 -> 0x4000
	A1 = A0 = 0;
	R5.H = (A1 += R0.H * R1.L), R5.L = (A0 += R0.H * R1.L) (FU);
	DBGA ( R5.L , 0x4000 );
	DBGA ( R5.H , 0x4000 );

// extraction with no saturation
// 0xffff * 0xffff = 0x00fffe0001 -> 0xfffe
	A1 = A0 = 0;
	R5.H = (A1 += R2.L * R2.L), R5.L = (A0 += R2.L * R2.L) (FU);
	DBGA ( R5.L , 0xfffe );
	DBGA ( R5.H , 0xfffe );

// extraction with saturation
//0x7ffffe0001 -> 0xffff
	A1 = A0 = 0;
	A1.x = R3.L;
	A0.x = R3.L;
	R5.H = (A1 += R2.L * R2.L), R5.L = (A0 += R2.L * R2.L) (FU);
	DBGA ( R5.L , 0xffff );
	DBGA ( R5.H , 0xffff );

	pass

	.data
data0:
	.dw 0x0001
	.dw 0x8000
	.dw 0x7fff
	.dw 0x8000
	.dw 0xffff
	.dw 0xf000
	.dw 0x007f
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
