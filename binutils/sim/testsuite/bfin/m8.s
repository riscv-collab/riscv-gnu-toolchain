//  MAC test program.
//  Test result extraction of mac instructions.
//  Test basic edge values
//  UNSIGNED INTEGER mode into SINGLE destination register
//  test ops: "+="
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x80000002
// load r1=0x80007fff
// load r2=0xf0000000
// load r3=0x0000007f
// load r4=0x00000080
// load r5=0xffffffff
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];
	R5 = [ P0 ++ ];

// 0x0002 * 0x0002 = 0x0000000004 -> 0x0004
	A1 = A0 = 0;
	R5.H = (A1 += R0.L * R0.L), R5.L = (A0 += R0.L * R0.L) (IU);
	DBGA ( R5.L , 0x4 );
	DBGA ( R5.H , 0x4 );

// 0x7fff * 0x007f = 0x00003f7f81 -> 0xffff
	A1 = A0 = 0;
	R5.H = (A1 += R1.L * R3.L), R5.L = (A0 += R1.L * R3.L) (IU);
	R5.H = (A1 += R1.L * R3.L), R5.L = (A0 += R1.L * R3.L) (IU);
	DBGA ( R5.L , 0xffff );
	DBGA ( R5.H , 0xffff );

	pass

	.data;
data0:
	.dw 0x0002
	.dw 0x8000
	.dw 0x7fff
	.dw 0x8000
	.dw 0x0000
	.dw 0xf000
	.dw 0x007f
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
	.dw 0xffff
	.dw 0xffff
