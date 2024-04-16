	.arch armv9.4-a+d128+the

	.macro	rwreg128, name
	mrrs	x2, x3, \name
	msrr	\name, x4, x5
	.endm

	rwreg128	par_el1
	rwreg128	rcwmask_el1
	rwreg128	rcwsmask_el1
	rwreg128	ttbr0_el1
	rwreg128	ttbr0_el12
	rwreg128	ttbr0_el2
	rwreg128	ttbr1_el1
	rwreg128	ttbr1_el12
	rwreg128	ttbr1_el2
	rwreg128	vttbr_el2
