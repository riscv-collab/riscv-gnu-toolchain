# mach: bfin

.include "testutils.inc"
	start

	r5=10;
	p1=r5;
	r7=20;
	lsetup (lstart, lend) lc0=p1;

lstart:
	nop;
	nop;
	nop;
	nop;
	jump lend;
	nop;
	nop;
	nop;
lend:
	r7 += -1;

	nop;
	nop;

	dbga( r7.l,10);

	pass
