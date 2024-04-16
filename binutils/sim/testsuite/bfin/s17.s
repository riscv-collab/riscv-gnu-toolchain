//  shifter test program.
//  Test instructions   ONES
# mach: bfin

.include "testutils.inc"
	start


	R7 = 0;
	ASTAT = R7;
	R0.L = 0x1;
	R0.H = 0x0;
	R7.L = ONES R0;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x0000;
	R0.H = 0x8000;
	R7.L = ONES R0;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

	R0.L = 0x0001;
	R0.H = 0x8000;
	R7.L = ONES R0;
	DBGA ( R7.L , 0x0002 );
	DBGA ( R7.H , 0x0000 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	R7.L = ONES R0;
	DBGA ( R7.L , 0x0020 );
	DBGA ( R7.H , 0x0000 );

	R0.L = 0x0000;
	R0.H = 0x0000;
	R7.L = ONES R0;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );

	pass
