# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	CC = BITTST ( R0 , 0x0 );
	BITSET( R0 , 0x0 );
	CC = BITTST ( R0 , 0x0 );
	CC = ! BITTST( R0 , 0x0 );
	R1.L = 1;
	R1.H = 0;
	CC = R0 == R1;
	CC = BITTST ( R0 , 0x1 );
	R5 = ASTAT;
	CHECKREG R5, (_AC0|_AC0_COPY|_AZ)

	BITSET( R0 , 0x1 );
	R5 = ASTAT;
	CHECKREG R5, 0
	CC = BITTST ( R0 , 0x1 );
	CC = ! BITTST( R0 , 0x1 );
	pass
