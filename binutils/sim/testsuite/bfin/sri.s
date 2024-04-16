# Blackfin testcase for BITMUX
# mach: bfin

	.include "testutils.inc"

	start

	r0 = 0;
	p2.l = 16;

ilp:
	BITMUX( R6 , R7, A0) (ASR);
	p2 += -1;
	cc=p2==0;
	if !cc jump ilp;
	A0 = A0 >> 8;
	R0 = A0.w;
	[ I1 ++ ] = R0;
	nop;

	pass
