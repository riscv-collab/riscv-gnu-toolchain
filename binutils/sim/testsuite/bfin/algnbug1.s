# mach: bfin

.include "testutils.inc"
	start


	loadsym P0, blocka;
	I0 = P0;

	DISALGNEXCPT || NOP || R0 = [ I0 ++ ];
	DISALGNEXCPT || NOP || R1 = [ I0 ++ ];

	DBGA ( R0.L , 0xfeff );
	DBGA ( R0.H , 0xfcfd );
	DBGA ( R1.L , 0xfafb );
	DBGA ( R1.H , 0xf8f9 );

	I0 = P0;
	M0 = 1 (X);
	I0 += M0;

	DISALGNEXCPT || NOP || R0 = [ I0 ++ ];
	DISALGNEXCPT || NOP || R1 = [ I0 ++ ];

	DBGA ( R0.L , 0xfeff );
	DBGA ( R0.H , 0xfcfd );
	DBGA ( R1.L , 0xfafb );
	DBGA ( R1.H , 0xf8f9 );

	pass

	.data
	.align 8
blocka:
	.dw 0xfeff
	.dw 0xfcfd
	.dw 0xfafb
	.dw 0xf8f9
