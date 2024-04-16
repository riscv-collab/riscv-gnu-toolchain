# mach: bfin

.include "testutils.inc"
	start

	loadsym p0, foo;
	r2 = p0;
	r2 += 4;
	[p0++]=p0;
	loadsym i0, foo;
	r0=[i0];
	R3 = P0;
	CC = R2 == R3
	if ! CC jump _fail;
	R3 = I0;
	CC = R0 == R3;
	if ! CC jump _fail;

_halt0:
	pass;
_fail:
	fail;

	.data
foo:
	.space (0x10)
