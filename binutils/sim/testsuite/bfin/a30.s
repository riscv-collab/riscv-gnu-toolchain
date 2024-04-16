# mach: bfin

.include "testutils.inc"
	start


	R0.L = 0.5;
	R0.H = 0.5;
	R1.L = 0.5;
	R1.H = 0.5;

	R2 = R0 +|+ R1, R3 = R0 -|- R1 (S , ASR);
	_DBGCMPLX R2;
	_DBGCMPLX R3;

	DBGA ( R2.L , 0.5 );
	DBGA ( R2.H , 0.5 );
	DBGA ( R3.L , 0 );
	DBGA ( R3.H , 0 );

	R1.L = 0.125;
	R1.H = 0.125;

	R2 = R0 +|+ R1, R3 = R0 -|- R1 (S , ASR);
	_DBGCMPLX R2;
	_DBGCMPLX R3;
	DBGA ( R2.L , 0.3125 );
	DBGA ( R2.H , 0.3125 );
	DBGA ( R3.L , 0.1875 );
	DBGA ( R3.H , 0.1875 );

	R0 = R2 +|+ R3, R1 = R2 -|- R3 (S , ASR);
	_DBGCMPLX R0;
	_DBGCMPLX R1;
	DBGA ( R0.L , 0.25 );
	DBGA ( R0.H , 0.25 );
	DBGA ( R1.L , 0.0625 );
	DBGA ( R1.H , 0.0625 );

	R0 = 1;
	R0 <<= 15;
	R1 = R0 << 16;
	r0=r0 | r1;
	R1 = R0;

	R2 = R0 +|+ R1, R3 = R0 -|- R1 (S , ASR);

	_DBGCMPLX R2;
	_DBGCMPLX R3;
	DBGA ( R0.L , 0x8000 );
	DBGA ( R0.H , 0x8000 );
	DBGA ( R1.L , 0x8000 );
	DBGA ( R1.H , 0x8000 );

	pass
