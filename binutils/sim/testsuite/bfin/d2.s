# Blackfin testcase for circular buffers and BREV
# mach: bfin

	.include "testutils.inc"

	start

	I0 = 0 (X);
	M0 = 0x8 (X);
	P0 = 16;
	loadsym R1, vals;

aaa:
	I0 += M0 (BREV);
	P0 += -1;

	R2 = I0;
	R0 = R1 + R2
	P1 = R0;
	R0 = B[P1] (Z);

	R3 = P0;

	CC = R0 == R3;
	if !CC JUMP _fail;

	CC = P0 == 0;
	IF !CC JUMP aaa (BP);
	R0 = I0;

	DBGA(R0.L, 0x0000);
	DBGA(R0.H, 0x0000);

	pass

_fail:
	fail

	.data
vals:
.db 0x0		/* 0 */
.db 0x8
.db 0xc
.db 0x4		/* 4 */
.db 0xe
.db 0x6
.db 0xa
.db 0x2		/* 8 */
.db 0xf
.db 0x7
.db 0xB
.db 0x3		/* c */
.db 0xD
.db 0x5
.db 0x9		/* f */
.db 0x1
