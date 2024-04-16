# mach: bfin

.include "testutils.inc"
	start

	loadsym i0, tmp0;

	r1 = i0;
	b0=i0;
	r3=4;
	l0=0;
	m0=0;

	r5.l=0xdead;
	r5.h=0xbeef;

	l0=r3;
	[i0++] = r5;
	l0 = 0;
	r0 = i0;

	CC = R0 == R1;
	if !CC JUMP _fail;

	l0=r3;
	r3=[i0--];
	r0=i0;

	CC = R0 == R1;
	if !CC JUMP _fail;

	pass

_fail:
	fail

	.data
tmp0:
	.space (0x100);
