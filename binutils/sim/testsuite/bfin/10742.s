# mach: bfin

.include "testutils.inc"
	start


	r5.h=0x1234;
	r5.l=0x5678;

	p5 = r5;
	p5.l = 0x1000;

	r0 = p5;
	dbga(r0.h, 0x1234);
	dbga(r0.l, 0x1000);

	pass
