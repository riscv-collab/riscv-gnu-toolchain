// testset instruction
//TESTSET is an atomic test-and-set.
//If the lock was not set prior to the TESTSET, cc is set, the lock bit is set,
//and this processor gets the lock. If the lock was set
//prior to the TESTSET, cc is cleared, the lock bit is still set,
//but the processor fails to acquire the lock.
# mach: bfin

	.include "testutils.inc"

	start

	loadsym P0, datalabel;

	R0 = 0;
	CC = R0;
	R0 = B [ P0 ] (Z);
	DBGA ( R0.L , 0 );
	TESTSET ( P0 );
	R0 = CC;
	DBGA ( R0.L , 1 );
	R0 = B [ P0 ] (Z);
	DBGA ( R0.L , 0x80 );

	R0 = 0;
	CC = R0;
	TESTSET ( P0 );
	R0 = CC;
	DBGA ( R0.L , 0 );
	R0 = B [ P0 ] (Z);
	DBGA ( R0.L , 0x80 );

	pass

	.data
datalabel:
	.dw 0
