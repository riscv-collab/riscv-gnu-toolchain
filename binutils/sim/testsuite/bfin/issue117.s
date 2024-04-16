# mach: bfin

.include "testutils.inc"
	start

// issue 117

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	A0 = 0;
	A1 = 0;
	R0.L = 0x0400;
	R1.L = 0x0010;
	R2.L = ( A0 = R0.L * R1.L ) (S2RND);

	DBGA ( R2.L , 0x1 );
	pass
