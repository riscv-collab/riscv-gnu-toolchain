# Blackfin testcase for playing with LSETUP
# mach: bfin

	.include "testutils.inc"

	start

	R0 = 0x123;
	P0 = R0;
	LSETUP (.L1, .L1) LC0 = P0;
.L1:
	R0 += -1;

	R1 = 0;
	CC = R1 == R0;
	IF CC JUMP 1f;
	fail
1:
	p0=10;
	loadsym i0,  _buf
	imm32 r0, 0x12345678
	LSETUP(.L2, .L3) lc0 = p0;
.L2:
	[i0++] = r0;
.L3:
	[i0++] = r0;

	loadsym R1, _buf
	R0 = 0x50;
	R1 = R0 + R1;
	R0 = I0;
	CC = R0 == R1;
	if CC JUMP 2f;
	fail
2:

	r5=10;
	p1=r5;
	r7=20;
	lsetup (.L4, .L5) lc0=p1;
.L4:
	nop;
	nop;
	nop;
	nop;
	jump .L5;
	nop;
	nop;
	nop;
.L5:
	r7 += -1;

	R0 = 10 (Z);
	CC = R7 == R0;
	if CC jump 3f;
	fail
3:
	r1 = 1;
	r2 = 2;
	r0 = 0;
	p1 = 10;
	loadsym p0, _buf;
	lsetup (.L6, .L7) lc0 = p1;
.L6:
	[p0++] = r1;
.L7:
	[p0++] = r2;

	r3 = P0;
	loadsym r1, _buf
	r0 = 80;
	r1 = r1 + r0;
	CC = R1 == R3
	if CC jump 4f;
	fail
4:

	R0 = 1;
	R1 = 2;
	R2 = 3;
	R4 = 4;
	P1 = R1;
	LSETUP (.L8, .L8) LC0 = P1;
	R5 = 5;
	R6 = 6;
	R7 = 7;
.L8:
	R1 += 1;

	R7 = 4;
	CC = R7 == R1;
	if CC jump 5f;
	fail
5:
	P1 = R1;
	LSETUP (.L9, .L9 ) LC1 = P1;
.L9:
	R1 += 1;
	R7 = 8;
	if CC jump 6f;
	fail
6:
	pass

.data
_buf:
	.rept 0x80
	.long 0
	.endr
