# mach: bfin

.include "testutils.inc"
	start

	r3.l=0x5678;
	r3.h=0x1234;

	p5=8;

	p5=r3;
	p5.l =4;

	r5=p5;
	dbga( r5.h, 0x1234);

_halt:
	pass;
