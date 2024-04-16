	.arch armv8.1-a+d128

	mrrs	x0, x1, accdata_el1
	msrr	accdata_el1, x0, x1
	mrrs	w0, w1, ttbr0_el1
	msrr	ttbr0_el1, w0, w1
	mrrs	x0, x2, ttbr0_el1
	msrr	ttbr0_el1, x0, x2
