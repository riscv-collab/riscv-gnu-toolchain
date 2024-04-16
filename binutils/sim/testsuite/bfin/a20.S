# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R1 = 0;
	ASTAT = R1;

	R1.H = -32768;
	R2 = 0;
	R2.H = -32768;
	R3 = R1 +|+ R2;
	_DBG ASTAT;
	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC1|_AZ);

	R0.L = 32767;
	R0.H = 32767;
	R0 = R0 +|- R0;
	_DBG ASTAT;
	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AN|_AZ)

	R0 = 0;
	ASTAT = R0;
	_DBG ASTAT;
	R7 = ASTAT;
	CHECKREG R7, (_UNSET)

	R1.L = -1;
	R1.H = 0x7fff;
	R0 = ABS R1;
	_DBG R0;
	_DBG ASTAT;
	R7 = ASTAT;
	CHECKREG R7, (_UNSET)

	R1=0;
	R1.H = 0x8000;
	_DBG R1;
	R0 = ABS R1;
	_DBG R0;
	_DBG ASTAT;
	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY)

	R0 = 0;
	ASTAT = R0;

	R1.L = 32767;
	R1.H = 32767;
	R0 = R1 +|+ R1 (CO);
	_DBG R0;
	_DBG ASTAT;
	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AN)

	R0.L = -1;
	R0.H = 32766;
	R1.L = -1;
	R1.H = -32768;
	R0 = PACK( R0.H , R1.L );
	_DBG R0;
	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AN)

	pass
