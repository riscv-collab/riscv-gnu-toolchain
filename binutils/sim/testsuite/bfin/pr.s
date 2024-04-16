# mach: bfin

.include "testutils.inc"
	start

	loadsym R3, foo;
	I1 = R3;

	R4 = 0x10
	R4 = R4 + R3;
	P0 = R4;

	R4 = 0x14;
	R4 = R4 + R3;
	I0 = R4;

	r0 = 0x22;
	loadsym P1, bar;

	[i0] = r0;
	[i1] = r0;

doItAgain:

	p2 = 4;
	r5=0;

	LSETUP ( lstart , lend) LC0 = P2;
lstart:

	MNOP  || R2 = [ I0 ++ ]  || R1 = [ I1 ++ ];
	CC = R1 == R2;
	IF CC JUMP lend;
	R1 = [ P1 + 0x0 ];
	R1 = R1 + R0;
	[ P1 + 0x0 ] = R1;

lend:
	NOP;

	if !cc jump _halt0;
	cc = r5 == 0;
	if !cc jump _halt0;

	r4=1;
	r5=r5+r4;
	r1=i0;
	R4 = 0x24;
	R4 = R3 + R4
	CC = R1 == R4
	if !CC JUMP _fail;

	i2=i0;
	r2=0x1234;
	[i2++]=r2;
	[i2++]=r2;
	[i2++]=r2;
	[i2++]=r2;
	[i2++]=r2;
	[i2++]=r2;
	[i2++]=r2;
	jump doItAgain;

_halt0:
	r0=i0;
	R4 = 0x34;
	R4 = R4 + R3;
	CC = R0 == R4;
	IF !CC JUMP _fail;

	pass;

_fail:
	fail;

	.data
foo:
	.space (0x100);

bar:
	.space (0x1000);
