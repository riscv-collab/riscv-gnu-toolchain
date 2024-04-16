# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	CC = R0 == R0;

	AZ = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AC0|_AC0_COPY|_CC|_AZ);
	R0 = R0 + R0;
	R0 = ASTAT;	CHECKREG R0, (_CC);

	AN = CC;
	R0 = ASTAT;	CHECKREG R0, (_CC|_AN);
	R0 = - R0;
	R0 = ASTAT;	CHECKREG R0, (_CC|_AN);

	AC0 = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AC0|_CC|_AN);

	AV0 = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AV0|_AC0|_CC|_AN);

	AV1 = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AV1|_AV0|_AC0|_CC|_AN);

	AQ = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AV1|_AV0|_AC0|_AQ|_CC|_AN);

	CC = R0 < R0;
	_DBG ASTAT;

// When AV0 is set, AV1 is unchanged
	AQ = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AV1|_AV0|_AC0|_AC0_COPY|_AZ);

	AV1 = CC;
	_DBG ASTAT;
	R0 = ASTAT;	CHECKREG R0, (_AV0|_AC0|_AC0_COPY|_AZ);

	pass
