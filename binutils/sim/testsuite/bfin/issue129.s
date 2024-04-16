# Blackfin testcase for PREGS and BREV
# mach: bfin

	.include "testutils.inc"

	start

// issue 129

	P0.L = 0x0000;
	P0.H = 0x8000;

	P4.L = 0x0000;
	P4.H = 0x8000;

	P4 += P0 (BREV);

	R0 = P4;
	DBGA ( R0.H , 0x4000 );
	DBGA ( R0.L , 0 );

//--------------

	P0.L = 0x0000;
	P0.H = 0xE000;

	P4.L = 0x1f09;
	P4.H = 0x9008;

	P4 += P0 (BREV);

	R0 = P4;
	DBGA ( R0.H , 0x0808 );
	DBGA ( R0.L , 0x1f09 );

	pass
