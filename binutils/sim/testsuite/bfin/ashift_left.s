# Blackfin testcase for left ashift
# Dreg = Dreg << imm (S);
# mach: bfin

	.include "testutils.inc"

	.macro test in:req, shift:req, out:req, opt
	imm32 r0, \in;
	r1 = r0 >>> \shift \opt;
	CHECKREG r1, \out;
	.endm

	start

test 2, 1, 1, (S);

	pass
