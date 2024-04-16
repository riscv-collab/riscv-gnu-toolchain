# mach: bfin

.include "testutils.inc"
	start

	r2.l = 0x1234;
	r2.h = 0xff90;

	r4=8;
	i2=r2;
	m2 = 4;
	a0 = 0;
	r1.l = (a0 += r4.l *r4.l) (IS) || I2 += m2 || nop;

	r0 = i2;

	dbga(r0.l, 0x1238);
	dbga(r0.h, 0xff90);

	pass
