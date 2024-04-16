# mach: bfin

.include "testutils.inc"
	start

	r0 = 0
	ASTAT = r0;

	r2=-7;
	r2+=-63;
	_dbg r2;
	_dbg astat;
	r7=astat;
	dbga ( r7.h, 0x0);
	dbga ( r7.l, 0x1006);

	r7=0;
	astat=r7;
	r2=64;
	r2+=-64;
	_dbg r2;
	_dbg astat;
	r7=astat;
	dbga ( r7.h, 0x0);
	dbga ( r7.l, 0x1005);

	r7=0;
	astat=r7;
	r2=0;
	r2.h=0x8000;
	r2+=-63;
	_dbg astat;
	_dbg r2;
	r7=astat;
	dbga ( r7.h, 0x0300);
	dbga ( r7.l, 0x100c);

	pass
