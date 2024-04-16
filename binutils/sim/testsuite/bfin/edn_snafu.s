# mach: bfin

.include "testutils.inc"
	start



	loadsym r7, foo;

	p0 = r7;

	r0.h=0x2a2a;
	r0.l=0x2a2a;

	[p0++]=r0;
	[p0++]=r0;
	r0=0;
	[p0++]=r0;

	p0 = r7;
	p1=-1;

	lsetup(lstart, lend) lc0=p1;

lstart:
	_dbg p0;
	r1=b[p0++] (z);
	cc = r1 == 0;
	if cc jump ldone;
lend:
	nop;

ldone:

	r1=b[p0++](z);
	r1=p0;
	r6 = r1 - r7;

	DBGA (R6.L, 0xA);

	pass;

	.data
foo:
	.space (0x100)
