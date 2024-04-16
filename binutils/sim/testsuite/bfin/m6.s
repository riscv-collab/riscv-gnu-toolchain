//  Test result extraction of mac instructions.
//  Test basic edge values
//  SIGNED INTEGER  mode into SINGLE destination register
//  test ops: "+="
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x80000001
// load r1=0x80007fff
// load r2=0xf0000000
// load r3=0x0000007f
// load r4=0x00000080
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// integer extraction with no saturation
// 0x1 * 0x1 = 0x0000000001 -> 0x1
	A1 = A0 = 0;
	R5.H = (A1 += R0.L * R0.L), R5.L = (A0 += R0.L * R0.L) (IS);
	DBGA ( R5.L , 0x1 );
	DBGA ( R5.H , 0x1 );

// integer extraction with positive saturation
// 0x7fff * 0x7f  -> 0x7fff
	A1 = A0 = 0;
	R5.H = (A1 += R1.L * R3.L), R5.L = (A0 += R1.L * R3.L) (IS);
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );

// integer extraction with negative saturation
// 0x8000 * 0x7f  -> 0x8000
	A1 = A0 = 0;
	R5.H = (A1 += R1.H * R3.L), R5.L = (A0 += R1.H * R3.L) (IS);
	DBGA ( R5.L , 0x8000 );
	DBGA ( R5.H , 0x8000 );

	pass

	.data;
data0:
	.dw 0x0001
	.dw 0x8000
	.dw 0x7fff
	.dw 0x8000
	.dw 0x0000
	.dw 0xf000
	.dw 0x007f
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
