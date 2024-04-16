# mach: bfin

.include "testutils.inc"
	start


	I0 = 0 (X);
	I1 = 0 (X);
	A0 = A1 = 0;
	init_r_regs 0;
	ASTAT = R0;

// This section of code will test the SAA instructions and sum of accumulators;

	loadsym I0, tstvecI;

	R0 = [ I0 ++ ];
	R2 = [ I0 ++ ];

// +++++++++++++++   TG11.001   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==> 15   15   15   15      //
//              r1 ==>  0    0    0    0      //
//                                            //
//       Output:r2 ==> 0    0    0   30       //
//              r3 ==> 0    0    0   30       //
// ++++++++++++++++++++++++++++++++++++++++++ //

	SAA ( R1:0 , R3:2 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x001e );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x001e );
	DBGA ( R7.H , 0x0000 );

	A1 = A0 = 0;

// +++++++++++++++   TG11.002   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==> 15   15   15   15      //
//              r1 ==>  0    0    0    0      //
//                                            //
//       Output:r2 ==> 0    0    0   30       //
//              r3 ==> 0    0    0   30       //
// ++++++++++++++++++++++++++++++++++++++++++ //

	SAA ( R1:0 , R3:2 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x001e );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x001e );
	DBGA ( R7.H , 0x0000 );

	A1 = A0 = 0;

// +++++++++++++++   TG11.003   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==> 240  240  240  240     //
//              r1 ==>  0    0    0    0      //
//                                            //
//       Output:r2 ==> 0    480               //
//              r3 ==> 0    480               //
// ++++++++++++++++++++++++++++++++++++++++++ //

	R0 = [ I0 ++ ];
	R2 = [ I0 ++ ];

	SAA ( R3:2 , R1:0 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x01e0 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x01e0 );
	DBGA ( R7.H , 0x0000 );

	A1 = A0 = 0;

// +++++++++++++++   TG11.004   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==> 240  240  240  240     //
//              r1 ==>  0    0    0    0      //
//                                            //
//       Output:r2 ==> 0    480               //
//              r3 ==> 0    480               //
// ++++++++++++++++++++++++++++++++++++++++++ //

	SAA ( R1:0 , R3:2 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x01e0 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x01e0 );
	DBGA ( R7.H , 0x0000 );

	A1 = A0 = 0;
// +++++++++++++++   TG11.005   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==>  0    0    0    0      //
//              r1 ==>  0    0    0    0      //
//                                            //
//       Output:r2 ==> 0    0                 //
//              r3 ==> 0    0                 //
// ++++++++++++++++++++++++++++++++++++++++++ //

	R0 = [ I0 ++ ];
	R2 = [ I0 ++ ];

	SAA ( R1:0 , R3:2 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );

// +++++++++++++++   TG11.006   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==> 255  255  255  255     //
//              r1 ==> 255  255  255  255     //
//                                            //
//       Output:r2 ==> 0    0                 //
//              r3 ==> 0    0                 //
// ++++++++++++++++++++++++++++++++++++++++++ //

	SAA ( R3:2 , R1:0 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );

	A1 = A0 = 0;

// +++++++++++++++   TG12.001   +++++++++++++ //
//                                            //
//                     HH   HL   LH   LL      //
//       Input: r0 ==> 255  255  255  255     //
//              r1 ==> 255  255  255  255     //
//                                            //
//       Output:r2 ==> 0    0                 //
//              r3 ==> 0    0                 //
// ++++++++++++++++++++++++++++++++++++++++++ //

	loadsym I0, tstvecK;
	B0 = I0;
	L0.L = 4;
	loadsym I1, tstvecJ;
	B1 = I1;
	L1.L = 4;

	P0 = 64 (X);
	R0 = [ I0 ++ ];
	R2 = [ I1 ++ ];
	LSETUP ( l$1 , l$1 ) LC0 = P0;
l$1:
	SAA ( R1:0 , R3:2 ) || R0 = [ I0 ++ ] || R1 = [ I1 ++ ];

	R2 = A1.L + A1.H, R3 = A0.L + A0.H;
	R7 = R2 + R3 (NS);
	DBGA ( R7.L , 0xff00 );
	DBGA ( R7.H , 0x0000 );

	R5.L = 0xfffa;
	A1 = R5;
	R5.H = 0xfff0;
	A0 = R5;

	loadsym I0, tstvecI;
	R0 = [ I0 ++ ];
	R2 = [ I0 ++ ];
	SAA ( R1:0 , R3:2 );
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;
	DBGA ( R6.L , 0x000e );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xfffe );
	DBGA ( R7.H , 0xffff );

	pass

	.data
tstvecI:
	.dw 0x0000
	.dw 0x0000
	.dw 0x0f0f
	.dw 0x0f0f
	.dw 0x0000
	.dw 0x0000
	.dw 0xf0f0
	.dw 0xf0f0
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff

	.data
tstvecJ:
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff
	.dw 0xffff

	.data
tstvecK:
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
