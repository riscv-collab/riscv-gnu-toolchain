# mach: bfin

.include "testutils.inc"
	start

// This function computes an integer 32x32 multiply,
// and returns the upper 32 bits of the result.
// If the complete 64 bit result is required, one must
// write the partial results as they are computed.
// To change this code for a fractional 32x32, one needs
// to adjust the shifts for magnitude of -15, and use a
// fractional multiply at the end for the upper word halves
// (instead of the integer one).

	loadsym P0, input_a;
	loadsym P1, input_b;
	loadsym P2, output;
	P4 = 10;
	LSETUP ( loop1 , loop1end ) LC0 = P4;
loop1:
	R0 = [ P0 ++ ];
	R1 = [ P1 ++ ];

			// begin integer double precision routine
			// 32 x 32 -> 32

	A1 = R0.H * R1.L (M), A0 = R0.L * R1.L (FU);
	A1 += R1.H * R0.L (M,IS);
	A0 = A0 >>> 16;
	A0 += A1;
	A0 = A0 >>> 16;
	A0 += R0.H * R1.H (IS);
	R7 = A0.w;

loop1end:
	[ P2 ++ ] = R7;	// store 32 bit output

	// test results
	loadsym P1, output;
	R0 = [ P1 ++ ];	DBGA ( R0.H , 0xfeae ); DBGA ( R0.L , 0xab6b );
	R0 = [ P1 ++ ];	DBGA ( R0.H , 0xfeae ); DBGA ( R0.L , 0xa627 );
	R0 = [ P1 ++ ];	DBGA ( R0.H , 0xfeae ); DBGA ( R0.L , 0xa0e3 );
	R0 = [ P1 ++ ];	DBGA ( R0.H , 0xfeae ); DBGA ( R0.L , 0x9b9f );
	pass

	.data
input_a:
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.dw 0x0000
	.dw 0xfabc
	.align 4;
input_b:
	.dw 0x1000
	.dw 0x4010
	.dw 0x1000
	.dw 0x4011
	.dw 0x1000
	.dw 0x4012
	.dw 0x1000
	.dw 0x4013
	.dw 0x1000
	.dw 0x4014
	.dw 0x1000
	.dw 0x4015
	.dw 0x1000
	.dw 0x4016
	.dw 0x1000
	.dw 0x4017
	.dw 0x1000
	.dw 0x4018
	.dw 0x1000
	.dw 0x4019
	.align 4;
output:
	.space (40);
