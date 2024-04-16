# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	R6.L = -32768;
	R6.H = -32768;
	R1.L = -32768;
	R1.H = -32768;

	R4 = R6.L * R1.H;

	_DBG ASTAT;

	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY);

	pass
