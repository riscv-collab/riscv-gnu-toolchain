// ACP 5.9 A0 -= A1 doesn't set flags
# mach: bfin

.include "testutils.inc"
	start

	A1 = A0 = 0;
	R0 = 0x0;
	astat=r0;
	A0.w = R0;
	R0.L = 0x0080;
	A0.x = R0;
	R1 = 1;

	_DBG A0;
	_DBG A1;

	A0 -= A1;
	_dbg A0;
	_dbg ASTAT;
	r7=astat;
	dbga (r7.h, 0x0);
	dbga (r7.l, 0x1006);

	A1 = A0 = 0;
	R0 = 0x1 (z);
	astat=r0;
	A0.w = R0;
	R0.L = 0x0080;
	A0.x = R0;
	R1 = 1;

	_DBG A0;
	_DBG A1;

	A0 -= A1;
	_dbg A0;
	_dbg ASTAT;
	r7=astat;
	dbga (r7.h, 0x0);
	dbga (r7.l, 0x1006);

	A1 = A0 = 0;
	R0 = 0x0;
	astat=r0;
	A0.w = R0;
	R0.L = 0x0080;
	A0.x = R0;
	R1 = 1;
	A1 = R1;

	_DBG A0;
	_DBG A1;

	A0 -= A1;
	_dbg A0;
	_dbg ASTAT;
	r7=astat;
	dbga (r7.h, 0x3);
	dbga (r7.l, 0x1006);

	A1 = A0 = 0;
	R0 = 0x1 (z);
	astat=r0;
	A0.w = R0;
	R0.L = 0x0080;
	A0.x = R0;
	R1 = 2 (z);
	A1 = R1;

	_DBG A0;
	_DBG A1;

	A0 -= A1;
	_dbg A0;
	_dbg ASTAT;
	r7=astat;
	dbga (r7.h, 0x3);
	dbga (r7.l, 0x1006);

	#

	A1 = A0 = 0;
	R0 = 0x0;
	astat=r0;
	R0.L=0xffff;
	R0.H=0xffff;
	A0.w = R0;
	R1=0x7f;
	A0.x = R1;
	A1.x = R1;
	A1.w = R0;

	_DBG A0;
	_DBG A1;

	A0 += A1;
	_dbg A0;
	_dbg ASTAT;
	r7=astat;
	dbga (r7.h, 0x3);
	dbga (r7.l, 0x0);

	A1 = A0 = 0;
	R0 = 0x0;
	astat=r0;
	A0.w = R0;
	R1=0x80;
	A0.x = R1;
	A1.x = R1;
	A1.w = R0;

	_DBG A0;
	_DBG A1;

	A0 += A1;
	_dbg A0;
	_dbg ASTAT;
	r7=astat;
	dbga (r7.h, 0x3);
	dbga (r7.l, 0x1006);

	pass;
