# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

// razor issue 146

	A0 = 0;
	A1 = 0;
	R0 = 0;
	ASTAT = R0;
	R1 = 0;
	R0.L = 0x891b;
	R0.H = 0x8537;
	R1.L = 0xab2d;
	R1.H = 0x3759;
	A0 = R0;
	A1 = R1;

	_DBG A0;
	_DBG A1;

	R3 = A1 + A0, R7 = A1 - A0 (S);
	_DBG R3;
	_DBG R7;

	_DBG ASTAT;
	R0 = ASTAT;
	CHECKREG R0, (_VS|_V|_V_COPY|_AN);

	pass
