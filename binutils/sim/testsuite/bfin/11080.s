# Blackfin testcase for DISALGNEXCPT
# mach: bfin

.include "testutils.inc"
	start

	loadsym R0, foo;
	R0 += 1;
	I1 = R0;

	M0 = 4 (z);

	//dag0misalgn, dag1misalgn EXCAUSE value
	R7 = 0x24 (z);

	// Get just the EXCAUSE field before
	R5=SEQSTAT;
	R5 = R5 << 26;
	R5 = R5 >> 26;

	DISALGNEXCPT || R2 = [I1++M0];	// i1 = 0xff9004aa (misaligned)

	// Get just the EXCAUSE field after
	R6=SEQSTAT;
	R6 = R6 << 26;
	R6 = R6 >> 26;

	// EXCAUSE of 0x24 == misaligned data memory access
	CC = R6 == R7;
	if CC jump _fail;

_pass:
	pass;

_fail:
	fail;

	.data
foo:
	.space 0x10
