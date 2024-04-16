	.arch armv9.4-a+d128

	/* TLBIP operands marked with the F_HASXT don not allow xzr to be used
	as GPR arguments and so require at least one register to be specified.  */
	tlbip	vale3nxs, x0
	tlbip	vale3nxs, x0, x1
	tlbip	vale3nxs, x2
	tlbip	vale3nxs, x2, x3

	/* No such checking is carried out when the same instruction is issued
	directly via the sysp implementation defined maintenance instruction,
	such that both GRPs are optional.  */
	sysp	#6, C9, C7, #5
	sysp	#6, C9, C7, #5, x0
	sysp	#6, C9, C7, #5, x0, x1
	sysp	#6, c9, c7, #5, x2
	sysp	#6, c9, c7, #5, x2, x3
