	.arch armv9.4-a+d128

	/* TLBIP operands marked with the F_HASXT don not allow xzr to be used
	as GPR arguments and so require at least one register to be specified.  */
	tlbip	vale3nxs
