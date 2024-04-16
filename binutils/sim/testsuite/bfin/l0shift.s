# mach: bfin

.include "testutils.inc"
	start


	r5 = 0;
	r2.L = 0xadbd;
	r2.h = 0xfedc;
	r5 = r2 >> 0;
	dbga (r5.l, 0xadbd);
	dbga (r5.h, 0xfedc);
	pass
