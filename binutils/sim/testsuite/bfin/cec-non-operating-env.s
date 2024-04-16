# Make sure the sim doesn't segfault when doing things that don't
# make much sense in a non-operating environment
# mach: bfin

	.include "testutils.inc"

	start

	csync;
	ssync;
	idle;
	raise 12;
	cli r0;
	sti r0;

	loadsym r0, .Lreti;
	reti = r0;
	rti;
	fail;
.Lreti:

	loadsym r0, .Lretx;
	retx = r0;
	rtx;
	fail;
.Lretx:

	loadsym r0, .Lretn;
	retn = r0;
	rtn;
	fail;
.Lretn:

	usp = p0;
	p0 = usp;

	pass;
