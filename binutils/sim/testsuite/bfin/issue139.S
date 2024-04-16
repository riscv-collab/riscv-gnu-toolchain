# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R6 = 0;
	R7 = 0;
	ASTAT = R0;
	R0.L = 0x33;
	R0.H = 0x55;
	R1.L = 0x66;
	R1.H = 0x77;
	R7 = R1 +|+ R0, R6 = R1 -|- R0 (SCO , ASR);

	_DBG R7;
	CHECKREG R7, 0x0066004c;
	CHECKREG R6, 0x00190011;
	R7 = ASTAT
	CHECKREG R7, 0;

//-----------------------

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R6 = 0;
	R7 = 0;
	R0.L = 0x33;
	R0.H = 0x55;
	R1.L = 0x66;
	R1.H = 0x77;
	R3 = R1 +|+ R0, R2 = R1 -|- R0 (ASR);

	R7 = ASTAT;
	CHECKREG R7, 0;

//-----------------------

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R6 = 0;
	R7 = 0;
	R0.L = 0x33;
	R0.H = 0x55;
	R1.L = 0x66;
	R1.H = 0x77;
	R5 = R1 +|+ R0, R4 = R1 -|- R0 (CO , ASR);

	R7 = ASTAT;
	CHECKREG R7, 0;

//-----------------------

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R6 = 0;
	R7 = 0;
	R0.L = 0x33;
	R0.H = 0x55;
	R1.L = 0x66;
	R1.H = 0x77;
	R3 = R1 +|+ R0, R2 = R1 -|- R0 (ASL);
	CHECKREG R3, 0x01980132;
	CHECKREG R2, 0x00440066;

	R7 = ASTAT;
	CHECKREG R7, 0;

//-----------------------

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R4 = 0;
	R5 = 0;
	R6 = 0;
	R7 = 0;
	R0.L = 0x33;
	R0.H = 0x55;
	R1.L = 0x7fff;
	R1.H = 0x77;
	R3 = R1 +|+ R0, R2 = R1 -|- R0 (S , ASL);
	CHECKREG R3, 0x01987fff;
	CHECKREG R2, 0x00447fff;

	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY);

	pass
