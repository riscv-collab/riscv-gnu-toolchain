# mach: bfin

// The assembly program uses two instructions to speed the decoder inner loop:
//     R6= VMAX/VMAX (R5, R4) A0>>2;
//     R2  =H+L (SGN(R0)*R1);
// VMAX is a 2-way parallel comparison of four updated path metrics, resulting
// in 2 new path metrics as well as a 2 bit field indicating the selection
// results. This 2 bit field is shifted into accumulator A0. This instruction
// implements the selections of a complete butterfly for a rate 1/n system.
// The H+L(SGN) instruction is used to compute the branch metric used by each
// butterfly. It takes as input a pair of values representing the received
// symbol, and another pair of values which are +1 or -1. The latter come
// from a pre-computed table that holds all the branch metric information for
// a specific set of polynomials. As all symbols are assumed to be binary,
// distance metrics between a received symbol and a branch metric are computed
// by adding and subtracting the values of the symbol according to the
// transition of a branch.

.include "testutils.inc"
	start

	// 16 in bytes for M2
	// A few pointer initializations
	// P2 points to decision history, where outputs are stored
	loadsym P2, DecisionHistory

	// P4 holds address of APMFrom
	loadsym P4, APMFrom;

	// P5 holds address of APMTo
	loadsym P5, APMTo;

	// I0 points to precomputed d's
	loadsym I0, BranchStorage;

	M2.L = 32;

	loadsym P0, InputData;

	// storage for all precomputed branch metrics
	loadsym P1, BranchStorage;

	R6 = 0;	R0 = 0;            	// inits

	R0.L = 0x0001;
	R0.H = 0x0001;
	[ P1 + 0 ] = R0;
	R0.L = 0xffff;
	R0.H = 0xffff;
	[ P1 + 4 ] = R0;
	R0.L = 0xffff;
	R0.H = 0x0001;
	[ P1 + 8 ] = R0;
	R0.L = 0x0001;
	R0.H = 0xffff;
	[ P1 + 12 ] = R0;
	R0.L = 0xffff;
	R0.H = 0x0001;
	[ P1 + 16 ] = R0;
	R0.L = 0x0001;
	R0.H = 0xffff;
	[ P1 + 20 ] = R0;
	R0.L = 0x0001;
	R0.H = 0x0001;
	[ P1 + 24 ] = R0;
	R0.L = 0xffff;
	R0.H = 0xffff;
	[ P1 + 28 ] = R0;
	R0.L = 0x0001;
	R0.H = 0xffff;
	[ P1 + 32 ] = R0;
	R0.L = 0xffff;
	R0.H = 0x0001;
	[ P1 + 36 ] = R0;
	R0.L = 0xffff;
	R0.H = 0xffff;
	[ P1 + 40 ] = R0;
	R0.L = 0x0001;
	R0.H = 0x0001;
	[ P1 + 44 ] = R0;
	R0.L = 0xffff;
	R0.H = 0xffff;
	[ P1 + 48 ] = R0;
	R0.L = 0x0001;
	R0.H = 0x0001;
	[ P1 + 52 ] = R0;
	R0.L = 0x0001;
	R0.H = 0xffff;
	[ P1 + 56 ] = R0;
	R0.L = 0xffff;
	R0.H = 0x0001;
	[ P1 + 60 ] = R0;

	P1 = 18;
	LSETUP ( L$0 , L$0end ) LC0 = P1;	// SymNo loop start

L$0:

	// Get a symbol and leave it resident in R1
	R1 = [ P0 ];	// R1=(InputData[SymNo*2+1] InputData[SymNo*2])
	P0 += 4;

	A0 = 0;

	// I0 points to precomputed D1, D0
	loadsym I0, BranchStorage;

	I1 = P4;	// I1 points to APM[From]
	I2 = P4;
	I2 += M2;	// I2 points to APM[From+16]
	I3 = P5;	// I3 points to APM[To]

	P1 = 16;
	P1 += -1;
	LSETUP ( L$1 , L$1end ) LC1 = P1;

	// APMFrom and APMTo are in alternate
	// memory banks.

	R0 = [ I0 ++ ];	// load R0 = (D1 D0)
	R3.L = W [ I1 ++ ];	// load RL3 = PM0
	// (R1 holds current symbol)

	R2.H = R2.L = SIGN(R0.H) * R1.H + SIGN(R0.L) * R1.L;	// apply sum-on-sign instruction
	R3.H = W [ I2 ++ ];	// now, R3 = (PM1 PM0)

L$1:
	R5 = R3 +|- R2 , R4 = R3 -|+ R2 || R0 = [ I0 ++ ] || NOP;
	//  R5 = (PM11 PM01) R4 = (PM10 PM00)
	// and load next (D1 D0)

	R6 = VIT_MAX( R5 , R4 ) (ASR) || R3.L = W [ I1 ++ ] || NOP;
	// do 2 ACS in parallel
	// R6 = (nPM1 nPM0)  and update to A0

L$1end:

	R2.H = R2.L = SIGN(R0.H) * R1.H + SIGN(R0.L) * R1.L || R3.H = W [ I2 ++ ] || [ I3 ++ ] = R6;
	// store new path metrics in
	// two consecutive locations

	R5 = R3 +|- R2 , R4 = R3 -|+ R2;

	R6 = VIT_MAX( R5 , R4 ) (ASR);

	[ I3 ++ ] = R6;

	R7 = A0.w;
	[ P2 ] = R7;
	P2 += 4;	// store history

	FP = P4;	// swap pointers From <--> To
	P4 = P5;
L$0end:
	P5 = FP;

	// check results
	loadsym I0, DecisionHistory

	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x6ff2 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0xf99f );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x9909 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x6666 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x0096 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x6996 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x9309 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x0000 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0xffff );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0xffff );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0xf0ff );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0xcf00 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x9009 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x07f6 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x6004 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x6996 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x8338 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x3443 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x6bd6 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x6197 );
	R0.L = W [ I0 ++ ];	DBGA ( R0.L , 0x6c26 );
	R0.H = W [ I0 ++ ];	DBGA ( R0.H , 0x0990 );

	pass

	.data
	.align 8
InputData:
	.dw 0x0001
	.dw 0x0001
	.dw 0xffff
	.dw 0xfffb
	.dw 0x0005
	.dw 0x0001
	.dw 0xfffd
	.dw 0xfffd
	.dw 0x0005
	.dw 0x0001
	.dw 0x0001
	.dw 0x0001
	.dw 0xffff
	.dw 0xfffb
	.dw 0x0005
	.dw 0x0001
	.dw 0xfffd
	.dw 0xfffd
	.dw 0x0005
	.dw 0x0001

	.align 8
APMFrom:
	.dw 0xc000
	.dw 0x0
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000
	.dw 0xc000

	.align 8
APMTo:
	.space (32*8)

	.align 8
BranchStorage:
	.space (32*8)

	.align 8
DecisionHistory:
	.space (18*4)
