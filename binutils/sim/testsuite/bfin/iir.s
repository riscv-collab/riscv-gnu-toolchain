# mach: bfin

// GENERIC BIQUAD:
// ---------------
//  x ---------+---------|---------+-------y
//             |         |t1       |
//             |         D         |
//             |   a1    |    b1   |
//             +---<-----|---->----+
//             |         |         |
//             |         D         |   D's are delays
//             |   a2    |    b2   |   ">" represent multiplications
//             +---<-----|---->----+
// To test this routine, use a biquad with a pole pair at z = (0.7 +- 0.1j),
// and a double zero at z = -1.0, which is a low-pass. The transfer function is:
//        1 + 2z^-1 + z^-2
// H(z) = ----------------------
//        1 - 1.4z^-1 + 0.5z^-2
// a1 = 1.4
// a2 = -0.5
// b1 = 2
// b2 = 1
// This filter conforms to the biquad test in BDT, since it has coefficients
// larger than 1.0 in magnitude, and b0=1. (Note that the a's have a negative
// sign.)
// This filter can be simulated in matlab. To simulate one biquad, use
// A = [1.0, -1.4, 0.5]
// B = [1, 2, 1]
// Y=filter(B,A,X)
// To simulate two cascaded biquads, use
// Y=filter(B,A,filter(B,A,X))
// SCALED COEFFICIENTS:
// --------------------
// In order to conform to 1.15 representation, must scale coeffs by 0.5.
// This requires an additional internal re-scale. The equations for the Type II
// filter are:
// t1 = x +     a1*t1*z^-1 + a2*t1*z^-2
// y  = b0*t1 + b1*t1*z^-1 + b2*t1*z^-2
// (Note inclusion of term b0, which in the example is b0 = 1.)
// If all coeffs are replaced by
// ai --> ai' = 0.5*a1
// then the two equations become
// t1    = x +     2*a1'*t1*z^-1 + 2*a2'*t1*z^-2
// 0.5*y = b0'*t1  + b1'*t1*z^-1 + b2'*t1*z^-2
// which can be implemented as:
//                2.0        b0'=0.5
//  x ---------+--->-----|---->----+-------y
//             |         |t1       |
//             |         D         |
//             |   a1'   |    b1'  |
//             +---<-----|---->----+
//             |         |         |
//             |         D         |
//             |   a2'   |    b2'  |
//             +---<-----|---->----+
// But, b0' can be eliminated by:
//  x ---------+---------|---------+-------y
//             |         |         |
//             |         V 2.0     |
//             |         |         |
//             |         |t1       |
//             |         D         |
//             |   a1'   |    b1'  |
//             +---<-----|---->----+
//             |         |         |
//             |         D         |
//             |   a2'   |    b2'  |
//             +---<-----|---->----+
// Function biquadf() computes this implementation on float data.
// CASCADED BIQUADS
// ----------------
// Cascaded biquads are simulated by simply cascading copies of the
// filter defined above. However, one must be careful with the resulting
// filter, as it is not very stable numerically (double poles in the
// vecinity of +1). It would of course be better to cascade different
// filters, as that would result in more stable structures.
// The functions biquadf() and biquadR() have been tested with up to 3
// stages using this technique, with inputs having small signal amplitude
// (less than 0.001) and under 300 samples.
//
// In order to pipeline, need to maintain two pointers into the state
// array: one to load (I0) and one to store (I2). This is required since
// the load of iteration i+1 is hoisted above the store of iteration i.

.include "testutils.inc"
	start


	// I3 points to input buffer
	loadsym I3, input;

	// P1 points to output buffer
	loadsym P1, output;

	R0 = 0;	R7 = 0;

	P2 = 10;
	LSETUP ( L$0 , L$0end ) LC0 = P2;
L$0:

	// I0 and I2 are pointers to state
	loadsym I0, state;
	I2 = I0;

	// pointer to coeffs
	loadsym I1, Coeff;

	R0.H = W [ I3 ++ ];	// load input value into RH0
	A0.w = R0;		// A0 holds x

	P2 = 2;
	LSETUP ( L$1 , L$1end ) LC1 = P2;

	// load 2 coeffs into R1 and R2
	// load state into R3
	R1 = [ I1 ++ ];
	MNOP || R2 = [ I1 ++ ] || R3 = [ I0 ++ ];

L$1:

	// A1=b1*s0   A0=a1*s0+x
	A1 = R1.L * R3.L, A0 += R1.H * R3.L || R1 = [ I1 ++ ] || NOP;

	// A1+=b2*s1  A0+=a2*s1
	// and move scaled value in A0 (t1) into RL4
	A1 += R2.L * R3.H, R4.L = ( A0 += R2.H * R3.H ) (S2RND) || R2 = [ I1 ++ ] || NOP;

	// Advance state. before:
	//  R4 =   uuuu   t1
	//  R3 = stat[1] stat[0]
	// after PACKLL:
	//  R3 = stat[0] t1
	R5 = PACK( R3.L , R4.L ) || R3 = [ I0 ++ ] || NOP;

	// collect output into A0, and move to RL0.
	// Keep output value in A0, since it is also
	// the accumulator used to store the input to
	// the next stage. Also, store updated state
L$1end:
	R0.L = ( A0 += A1 ) || [ I2 ++ ] = R5 || NOP;

	// store output
L$0end:
	W [ P1 ++ ] = R0;

	// Check results
	loadsym I2, output;
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x0028 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x0110 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x0373 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x075b );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x0c00 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x1064 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x13d3 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x15f2 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x16b9 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x1650 );

	pass

	.data
state:
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000

	.data
Coeff:
	.dw 0x7fff
	.dw 0x5999
	.dw 0x4000
	.dw 0xe000
	.dw 0x7fff
	.dw 0x5999
	.dw 0x4000
	.dw 0xe000
input:
	.dw 0x0028
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
output:
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
