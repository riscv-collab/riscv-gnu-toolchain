# mach: bfin

.include "testutils.inc"
	start

	p0=10;
	loadsym i0, foo;

	R2 = i0;
	r0.l = 0x5678;
	r0.h = 0x1234;

	lsetup(lstart, lend) lc0=p0;

lstart:
	[i0++] = r0;
lend:
	[i0++] = r0;

	r0=i0;
	R0 = R0 - R2;
	dbga(r0.l, 0x0050);

	pass

	.data
foo:
	.space (0x100)
