# mach: bfin

.include "testutils.inc"
	start

	r0.h=0xa5a5;
	r0.l=0xffff;
	a0 = 0;
	r0=a0.x;
	dbga(r0.h, 0x0000);
	dbga(r0.l, 0x0000);
	pass;
