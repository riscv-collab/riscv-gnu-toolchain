// Assert that loops can have coincidental loop ends.
# mach: bfin

.include "testutils.inc"
	start


	P0 = 3;
	R1 = 0;
	LSETUP ( out0 , out1 ) LC0 = P0;
out0:
	LSETUP ( out1 , out1 ) LC1 = P0;
out1:
	R1 += 1;

	DBGA ( R1.L , 9 );
	pass
