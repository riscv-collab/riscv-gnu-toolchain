# mach: bfin

// GENERIC PN SEQUENCE GENERATOR
// Linear Feedback Shift Register
// -------------------------------
// This solution implements an LFSR by applying an XOR reduction
// function to the 40 bit accumulator, XORing the contents of the
// CC bit, shifting by one the accumulator, and inserting the
// resulting bit on the open bit slot.
//  CC --> ----- XOR--------------------------
// 	   |          |     |     |  |	     |
// 	   |          |     |     |  |	     |
//        +------------------------------+   v
//        | b0 b1 b2 b3          b38 b39 |  in <-- by one
//        +------------------------------+
// after:
//        +------------------------------+
//        | b1 b2 b3          b38 b39 in |
//        +------------------------------+
// The program shown here is a PN sequence generator, and hence
// does not take any input other than the initial state. However,
// in order to accept an input, one simply needs to rotate the
// input sequence via CC prior to applying the XOR reduction.

.include "testutils.inc"
	start

	loadsym P1, output;
	init_r_regs 0;
	ASTAT = R0;

// load Polynomial into  A1
	A1 = A0 = 0;
	R0.L = 0x1cd4;
	R0.H = 0xab18;
	A1.w = R0;
	R0.L = 0x008d;
	A1.x = R0.L;

// load InitState into  A0
	R0.L = 0x0001;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;

	P4 = 4;
	LSETUP ( l$0 , l$0end ) LC0 = P4;
	l$0:                          	// **** START l-LOOP *****

	P4 = 32;
	LSETUP ( m$1 , m$1 ) LC1 = P4;	// **** START m-LOOP *****
	m$1:
	A0 = BXORSHIFT( A0 , A1, CC );

// store 16 bits of outdata RL1
	R1 = A0.w;
	l$0end:
	[ P1 ++ ] = R1;

// Check results
	loadsym I2, output;
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x5adf );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x2fc9 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0xbd91 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x5520 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x80d5 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x7fef );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x34d1 );
	R0.L = W [ I2 ++ ];	DBGA ( R0.L , 0x915c );
	pass

	.data;
output:
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
