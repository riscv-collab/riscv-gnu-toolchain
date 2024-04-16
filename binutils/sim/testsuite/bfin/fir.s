# mach: bfin

// FIR FILTER COMPTUED DIRECTLY ON INPUT WITH NO
//   INTERNAL STATE
//   TWO OUTPUTS PER ITERATION
// This program computes a FIR filter without maintaining a buffer of internal
// state.
// This example computes two output samples per inner loop. The following
// diagram shows the alignment required for signal x and coefficients c:
// x0 x1 x2 x3 x4 x5
// c0 c1 c2 c3 c4      -> output z(0)=x0*c0 + x1*c1 + ...
//    c0 c1 c2 c3 c4   ->        z(1)=x1*c0 + x2*c1 + ...
//	       L-1
//               ---
//      Z(k) =   \   c(n) * x(n+k)
//               /
//	         ---
//       	       n=0
// Naive, first stab at spliting this for dual MACS.
//	       L/2-1                     L/2-1
//               --- 		           ---
//      R(k) =   \   (x(2n) * y(2n+k))  +  \   (x(2n-1) * y(2n-1+k))
//               /  		           /
//	         --- 		           ---
//       	       n=0		         n=0
// Alternate, better partitioning for the machine.
//	       L-1
//               ---
//      R(0) =   \   x(n) * y(n)
//               /
//	         ---
//       	 n=0
//	       L-1
//               ---
//      R(1) =   \   x(n) * y(n+1)
//               /
//	         ---
//              n=0
//	       L-1
//               ---
//      R(2) =   \   x(n) * y(n+2)
//               /
//	         ---
//              n=0
//	       L-1
//               ---
//      R(3) =   \   x(n) * y(n+3)
//               /
//	         ---
//               n=0
//		.
//		.
//		.
//		.
// Okay in this verion the inner loop will compute R(2k) and R(2k+1) in parallel
//	       L-1
//               ---
//     R(2k) =   \   x(n) * y(n+2k)
//               /
//	         ---
//              n=0
//	       L-1
//               ---
//   R(2k+1) =   \   x(n) * y(n+2k+1)
//               /
//	         ---
//              n=0
// Implementation
// --------------
// Sample pair x1 x0 is loaded into register R0, and coefficients c1 c0
// is loaded into register R1:
// +-------+ R0
// | x1 x0 |
// +-------+
// +-------+ R1
// | c1 c0 |  compute two MACs: z(0)+=x0*c0, and z(1)+=x1*c0
// +-------+
// Now load x2 into lo half of R0, and compute the next two MACs:
// +-------+ R0
// | x1 x2 |
// +-------+
// +-------+ R1
// | c1 c0 |    compute z(0)+=x1*c1 and z(1)+=x2*c1 (c0 not used)
// +-------+
// Meanwhile, load coefficient pair c3 c2 into R2, and x3 into hi half of R0:
// +-------+ R0
// | x3 x2 |
// +-------+
// +-------+ R2
// | c3 c2 |    compute z(0)+=x2*c2 and z(1)+=x3*c2 (c3 not used)
// +-------+
// Load x4 into low half of R0:
// +-------+ R0
// | x3 x4 |
// +-------+
// +-------+ R1
// | c3 c2 |    compute z(0)+=x3*c3 and z(1)+=x4*c3 (c2 not used)
// +-------+
// //This is a reference FIR function used to test: */
//void firf (float input[], float  output[], float coeffs[],
//           long input_size, long coeffs_size)
//{
//  long i, k;
//  for(i=0;	i< input_size; i++){
//    output[i] = 0;
//    for(k=0;	k < coeffs_size; k++)
//	output[i] += input[k+i] * coeffs[k];
// }
//}

.include "testutils.inc"
	start


	R0 = 0;	R1 = 0; R2 = 0;
	P1 = 128 (X);	// Load loop bounds in R5, R6, and divide by 2
	P2 = 64 (X);

	// P0 holds pointer to input data in one memory
	// bank. Increments by 2 after each inner-loop iter
	loadsym P0, input;

	// Pointer to coeffs in alternate memory bank.
	loadsym I1, coef;

	// Pointer to outputs in any memory bank.
	loadsym I2, output;

	// Setup outer do-loop for M/2 iterations
	// (2 outputs are computed per pass)

	LSETUP ( L$0 , L$0end ) LC0 = P1 >> 1;

L$0:
	loadsym I1, coef;
	I0 = P0;
		// Set-up inner do-loop for L/2 iterations
		// (2 MACs are computed per pass)

	LSETUP ( L$1 , L$1end ) LC1 = P2 >> 1;

		// Load first two data elements in r0,
		// and two coeffs into r1:

	R0.L = W [ I0 ++ ];
	A1 = A0 = 0 || R0.H = W [ I0 ++ ] || R1 = [ I1 ++ ];

L$1:
	A1 += R0.H * R1.L, A0 += R0.L * R1.L || R0.L = W [ I0 ++ ] || NOP;
L$1end:
	A1 += R0.L * R1.H, A0 += R0.H * R1.H || R0.H = W [ I0 ++ ] || R1 = [ I1 ++ ];

	// Line 1: do 2 MACs and load next data element into RL0.
	// Line 2: do 2 MACs, load next data element into RH0,
	// and load next 2 coeffs

	R0.H = A1, R0.L = A0;

		// advance data pointer by 2 16b elements
	P0 += 4;

L$0end:
	[ I2 ++ ] = R0;	// store 2 outputs

	// Check results
	loadsym I2, output;

	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x0800 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x1000 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x2000 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x1000 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x0800 );
	pass

	.data
input:
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x4000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.space ((128-10)*2);	// must pad with zeros or uninitialized values.

	.data
coef:
	.dw 0x1000
	.dw 0x2000
	.dw 0x4000
	.dw 0x2000
	.dw 0x1000
	.dw 0x0000
	.space ((64-6)*2);	// must pad with zeros or uninitialized values.

	.data
output:
	.space (128*4)
