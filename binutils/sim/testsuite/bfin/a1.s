// check the imm7 bit constants bounds
# mach: bfin

.include "testutils.inc"
	start

	R0 = 63;
	DBGA ( R0.L , 63 );
	R0 = -64;
	DBGA ( R0.L , 0xffc0 );
	P0 = 63;
	R0 = P0;	DBGA ( R0.L , 63 );
	P0 = -64;
	R0 = P0;	DBGA ( R0.L , 0xffc0 );

// check loading imm16 into h/l halves
	R0.L = 0x1111;
	DBGA ( R0.L , 0x1111 );

	R0.H = 0x1111;
	DBGA ( R0.H , 0x1111 );

	P0.L = 0x2222;
	R0 = P0;	DBGA ( R0.L , 0x2222 );

	P0.H = 0x2222;
	R0 = P0;	DBGA ( R0.H , 0x2222 );

	pass
