// simple test to ensure that we can load data from memory.
# mach: bfin

.include "testutils.inc"
	start

	loadsym P0, tab;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];
	R5 = [ P0 ++ ];
	R6 = [ P0 ++ ];
	R7 = [ P0 ++ ];

	DBGA ( R0.H , 0x1111 );
	DBGA ( R1.H , 0x2222 );
	DBGA ( R2.H , 0x3333 );
	DBGA ( R3.H , 0x4444 );
	DBGA ( R4.H , 0x5555 );
	DBGA ( R5.H , 0x6666 );
	DBGA ( R6.H , 0x7777 );
	DBGA ( R7.H , 0x8888 );

	loadsym P0, tab2;

	R0 = W [ P0 ++ ] (Z);
	DBGA ( R0.L , 0x1111 );

	R1 = W [ P0 ++ ] (Z);
	DBGA ( R1.L , 0x8888 );

	R2 = W [ P0 ++ ] (Z);
	DBGA ( R2.L , 0x2222 );

	R3 = W [ P0 ++ ] (Z);
	DBGA ( R3.L , 0x7777 );

	R4 = W [ P0 ++ ] (Z);
	DBGA ( R4.L , 0x3333 );

	R5 = W [ P0 ++ ] (Z);
	DBGA ( R5.L , 0x6666 );

	R0 = B [ P0 ++ ] (Z);
	DBGA ( R0.L , 0x44 );
	R1 = B [ P0 ++ ] (Z);
	DBGA ( R1.L , 0x44 );
	R2 = B [ P0 ++ ] (Z);
	DBGA ( R2.L , 0x55 );
	R3 = B [ P0 ++ ] (Z);
	DBGA ( R3.L , 0x55 );

	R0 = B [ P0 ++ ] (X);
	DBGA ( R0.L , 0x55 );

	R1 = B [ P0 ++ ] (X);
	DBGA ( R1.L , 0x55 );

	R0 = W [ P0 ++ ] (X);
	DBGA ( R0.L , 0x4444 );

	R1 = [ P0 ++ ];
	DBGA ( R1.L , 0x6666 );
	DBGA ( R1.H , 0x3333 );

	P1 = [ P0 ++ ];
	R0 = P1;
	DBGA ( R0.L , 0x7777 );
	DBGA ( R0.H , 0x2222 );

	P1 = [ P0 ++ ];
	R0 = P1;
	DBGA ( R0.L , 0x8888 );
	DBGA ( R0.H , 0x1111 );

	loadsym P5, tab3;

	R0 = B [ P5 ++ ] (X);
	DBGA ( R0.H , 0 );
	DBGA ( R0.L , 0 );

	R0 = B [ P5 ++ ] (X);
	DBGA ( R0.H , 0xffff );
	DBGA ( R0.L , 0xffff );

	R1 = W [ P5 ++ ] (X);
	DBGA ( R1.H , 0xffff );
	DBGA ( R1.L , 0xffff );

	pass

	.data
tab:
	.dw 0
	.dw 0x1111
	.dw 0
	.dw 0x2222
	.dw 0
	.dw 0x3333
	.dw 0
	.dw 0x4444
	.dw 0
	.dw 0x5555
	.dw 0
	.dw 0x6666
	.dw 0
	.dw 0x7777
	.dw 0
	.dw 0x8888
	.dw 0
	.dw 0
	.dw 0
	.dw 0

tab2:
	.dw 0x1111
	.dw 0x8888
	.dw 0x2222
	.dw 0x7777
	.dw 0x3333
	.dw 0x6666
	.dw 0x4444
	.dw 0x5555
	.dw 0x5555
	.dw 0x4444
	.dw 0x6666
	.dw 0x3333
	.dw 0x7777
	.dw 0x2222
	.dw 0x8888
	.dw 0x1111

tab3:
	.dw 0xff00
	.dw 0xffff
