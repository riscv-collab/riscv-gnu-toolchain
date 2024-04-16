# mach: bfin

.include "testutils.inc"
	start


	/* Stall tests */

	r0 = 0;
	r1 = 1;
	loadsym p0, foo;
	p1 = p0;

pass_1:
	cc = r0;
	nop;
	nop;

	if cc jump _fail_1;
	[p0++] = p0;
	[p0++] = p0;
	r7 = p0;
	r5 = CC;
	P1 += 8;
	r6 = p1;
	CC = R6 == R7;
	if !CC jump _failure;

	cc = R5;
	if !cc jump over;

_fail_1:
	[p0++] = p0;
	[p0++] = p0;

back:
	if !cc jump skip(bp);

_fail_2:
	[p0++] = p0;
	[p0++] = p0;

over:
	if cc jump _fail_3(bp);
	[p0++] = p0;
	[p0++] = p0;
	r7=p0;
	R5=cc;
	P1 += 8;
	R6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;

	CC = R5;
	if !cc jump back(bp);

_fail_3:
	[p0++] = p0;
	[p0++] = p0;

skip:
	[p0++] = p0;
	[p0++] = p0;
	[p0++] = p0;
	r7=p0;

	P1 += 0xc;
	R6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;

next:
	[p0++] = p0;
	r7=p0;
	P1 += 4;
	R6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;

pass_2:
	cc = r1;
	nop;
	nop;

	if !cc jump _fail_4;
	[p0++] = p0;
	[p0++] = p0;
	r7=p0;
	R5 = cc;
	P1 += 8;
	R6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;

	cc = R5;
	if cc jump over_2;

_fail_4:
	[p0++] = p0;
	[p0++] = p0;
	P1 += 8;

back_2:
	if cc jump skip_2 (bp);

_fail_5:
	[p0++] = p0;
	[p0++] = p0;
	P1 += 8;

over_2:
	if !cc jump _fail_6 (bp);
	[p0++] = p0;
	[p0++] = p0;
	r7=p0;
	R5 = cc;
	P1 += 8;
	R6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;
	cc = R5;

	if cc jump back_2 (bp);

_fail_6:
	[p0++] = p0;
	[p0++] = p0;

skip_2:
	[p0++] = p0;
	[p0++] = p0;
	[p0++] = p0;
	r7=p0;
	R5 = cc;
	P1 += 0xc;
	R6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;
	cc = r5;

	if cc jump next_2 (bp);

next_2:
	[p0++] = p0;
	[p0++] = p0;
	P1 += 8;
	r7=p0;
	r6 = P1;
	CC = R6 == R7;
	if !CC jump _failure;

	cc = r0;
_halt:
	pass;

_fail_7:
	[p0++] = p0;

_failure:
	fail;

	.data
foo:
	.space (0x100)
