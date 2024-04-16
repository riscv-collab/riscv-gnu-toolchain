/*  DAG test program.
 *  Test circular buffers
 */
# mach: bfin

.include "testutils.inc"
	start

	loadsym I0, foo;
	loadsym B0, foo;
	loadsym R2, foo;
	L0 = 0x10 (X);
	M1 = 8 (X);
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2
	DBGA ( R1.L , 0x0008 );
	R0 = [ I0 ++ M1 ];
	R7 = I0;

	R1 = R7 - R2;
	DBGA ( R1.L , 0x0000 );
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2
	DBGA ( R1.L , 0x0008 );

	loadsym I0, foo;
	loadsym B0, foo;
	loadsym R2, foo;
	L0 = 0x10 (X);
	M1 = -4 (X);
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2
	DBGA ( R1.L , 0x000c );
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2
	DBGA ( R1.L , 0x0008 );
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0004 );
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0000 );
	R0 = [ I0 ++ M1 ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x000c );

	loadsym I0, foo;
	loadsym B0, foo;
	loadsym R2, foo;
	L0 = 0x8 (X);
	R0 = [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0004 );
	R0 = [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0000 );
	R0 = [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0004 );

	loadsym I0, foo;
	loadsym B0, foo;
	loadsym R2, foo;
	L0 = 0x8 (X);
	R0.L = W [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0002 );
	R0.L = W [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0004 );
	R0.L = W [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0006 );
	R0.L = W [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0000 );
	R0.L = W [ I0 ++ ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0002 );

	loadsym I0, foo;
	loadsym B0, foo;
	loadsym R2, foo;
	L0 = 0x8 (X);
	R0 = [ I0 -- ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0004 );
	R0 = [ I0 -- ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0000 );
	R0 = [ I0 -- ];
	R7 = I0;
	R1 = R7 - R2;
	DBGA ( R1.L , 0x0004 );

	pass

	.data
foo:
	.space (0x10);
