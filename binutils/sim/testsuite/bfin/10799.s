# mach: bfin

.include "testutils.inc"
	start

	fp = sp;

	[--SP]=RETS;

	loadsym R1, _b;
	loadsym R2, _a;
	R0 = R2;

	SP +=  -12;
	R2 =   4;

	CALL _dot;
	R1 = R0;

	R0 =   30;
	dbga( r1.l, 0x1e);


	pass

_dot:
	P0 = R1;
	CC = R2 <=  0;
	R3 = R0;
	R0 =   0;
	IF CC JUMP  ._P1L1 (bp);
	R0 =   1;
	I0 = R3;
	R0 = MAX (R0,R2) || R2 = [P0++] || NOP;
	P1 = R0;
	R0 = 0;
	R1 = [I0++];
	LSETUP (._P1L4 , ._P1L5) LC0=P1;

._P1L4:
	R1 *= R2;
._P1L5:
	R0= R0 + R1 (NS) || R2 = [P0++] || R1 = [I0++];

._P1L1:
	RTS;

.data;
_a:
	.db 0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00;
	.db 0x04,0x00,0x00,0x00;

_b:
	.db 0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00;
	.db 0x04,0x00,0x00,0x00;
