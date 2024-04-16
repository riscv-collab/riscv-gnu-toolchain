//  Test  byte-align instructions
# mach: bfin

.include "testutils.inc"
	start


	R0.L = 0xabcd;
	R0.H = 0x1234;
	R1.L = 0x4567;
	R1.H = 0xdead;

	R2 = ALIGN8 ( R1 , R0 );
	DBGA ( R2.L , 0x34ab );
	DBGA ( R2.H , 0x6712 );

	R2 = ALIGN16 ( R1 , R0 );
	DBGA ( R2.L , 0x1234 );
	DBGA ( R2.H , 0x4567 );

	R2 = ALIGN24 ( R1 , R0 );
	DBGA ( R2.L , 0x6712 );
	DBGA ( R2.H , 0xad45 );

	pass
