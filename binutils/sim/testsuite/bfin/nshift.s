// ACP 5.18: Shifter uses wrong shift value
# mach: bfin

.include "testutils.inc"
	start


	r0=0;
	r0.h=0x8000;
	r1=0x20 (z);
	r0 >>>= r1;
	dbga (r0.h, 0xffff);
	dbga (r0.l, 0xffff);

	r0=0;
	r0.h=0x7fff;
	r0 >>>= r1;
	dbga (r0.h, 0x0000);
	dbga (r0.l, 0x0000);

	r0.l=0xffff;
	r0.h=0xffff;
	r0 >>= r1;
	dbga (r0.h, 0x0000);
	dbga (r0.l, 0x0000);

	r0.l=0xffff;
	r0.h=0xffff;
	r0 <<= r1;
	dbga (r0.h, 0x0000);
	dbga (r0.l, 0x0000);

	pass;
