//  test program for microcontroller instructions
//  Test instructions
//  r4 = r2 & r3;
//  r4 = r2 | r3;
//  r4 = r2 ^ r3;
//  r4 = ~ r2;
# mach: bfin

.include "testutils.inc"
	start

	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

	R7 = R0 & R1;
	DBGA ( R7.L , 0x1111 );
	DBGA ( R7.H , 0x1111 );

	R7 = R2 & R3;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

	R7 = R0 | R1;
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0xffff );

	R7 = R2 | R3;
	DBGA ( R7.L , 0x000f );
	DBGA ( R7.H , 0x0000 );

	R7 = R0 ^ R1;
	DBGA ( R7.L , 0xeeee );
	DBGA ( R7.H , 0xeeee );

	R7 = R2 ^ R3;
	DBGA ( R7.L , 0x000e );
	DBGA ( R7.H , 0x0000 );

	R7 = ~ R0;
	DBGA ( R7.L , 0xeeee );
	DBGA ( R7.H , 0xeeee );

	R7 = ~ R2;
	DBGA ( R7.L , 0xfffe );
	DBGA ( R7.H , 0xffff );

	pass

	.data
data0:
	.dw 0x1111
	.dw 0x1111
	.dw 0xffff
	.dw 0xffff
	.dw 0x0001
	.dw 0x0000
	.dw 0x000f
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
