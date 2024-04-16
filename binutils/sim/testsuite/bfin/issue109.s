//Statement of problem...
//16-bit ashift and lshift uses a 6-bit signed  magnitude, which gives a
//range from -32 to 31. test the boundary.
# mach: bfin

.include "testutils.inc"
	start


	R1.L = 0x8000;
	R0.L = -32;
	R2.L = ASHIFT R1.L BY R0.L;

	DBGA ( R2.L , 0xffff );

	pass
