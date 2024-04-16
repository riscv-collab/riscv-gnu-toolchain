# Blackfin testcase for playing with TESTSET
# mach: bfin

	.include "testutils.inc"

	start

	loadsym P0, element1

	loadsym P1, element2

	R0 = B [P0];			// R0 should get 00
	R1 = B [P1];			// R1 should get 02

	TESTSET(P0);		    // should set CC and MSB of memory byte
	R0 = CC;
	TESTSET(P1);			// should clear CC and not change MSB of memory
	R1 = CC;

	R2 = B [P0];			// R2 should get 80
	R3 = B [P1];			// R3 should get 02

	dbga(R0.l,0x0001);
	dbga(R0.h,0x0000);
	dbga(R1.l,0x0000);
	dbga(R1.h,0x0000);
	dbga(R2.l,0x0080);
	dbga(R2.h,0x0000);
	dbga(R3.l,0x0082);
	dbga(R3.h,0x0000);

	pass

.data
.align 4;
element1: .long 0x0
element2: .long 0x2
element3: .long 0x4
