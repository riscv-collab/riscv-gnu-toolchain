# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;
	imm32 R1, 0x80007fff;
	imm32 R0, 0x00010001;
	R0 = R1 +|+ R0, R2 = R1 -|- R0 (S , ASL);
	_DBG R0;
	_DBG R2;
	CHECKREG R0, 0x80007fff;
	CHECKREG R2, 0x80007fff;

	R0 = ASTAT;
	_dbg r0;
	DBGA ( R0.L , 0x000a );
	DBGA ( R0.H , 0x0300 );

	R0 = 0;
	R1 = 0;
	R4 = 0;
	ASTAT = R0;
	R4 = R1 +|+ R0, R0 = R1 -|- R0 (S , ASL);
	_DBG R4;
	_DBG R0;
	R7 = ASTAT;
	_DBG R7;
	_DBG ASTAT;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

	pass
