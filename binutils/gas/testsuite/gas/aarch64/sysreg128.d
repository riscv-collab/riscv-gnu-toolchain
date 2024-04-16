#objdump: -dr

.*


Disassembly of section \.text:

0+ <\.text>:
[^:]*:	d5787402 	mrrs	x2, x3, par_el1
[^:]*:	d5587404 	msrr	par_el1, x4, x5
[^:]*:	d578d0c2 	mrrs	x2, x3, rcwmask_el1
[^:]*:	d558d0c4 	msrr	rcwmask_el1, x4, x5
[^:]*:	d578d062 	mrrs	x2, x3, rcwsmask_el1
[^:]*:	d558d064 	msrr	rcwsmask_el1, x4, x5
[^:]*:	d5782002 	mrrs	x2, x3, ttbr0_el1
[^:]*:	d5582004 	msrr	ttbr0_el1, x4, x5
[^:]*:	d57d2002 	mrrs	x2, x3, ttbr0_el12
[^:]*:	d55d2004 	msrr	ttbr0_el12, x4, x5
[^:]*:	d57c2002 	mrrs	x2, x3, ttbr0_el2
[^:]*:	d55c2004 	msrr	ttbr0_el2, x4, x5
[^:]*:	d5782022 	mrrs	x2, x3, ttbr1_el1
[^:]*:	d5582024 	msrr	ttbr1_el1, x4, x5
[^:]*:	d57d2022 	mrrs	x2, x3, ttbr1_el12
[^:]*:	d55d2024 	msrr	ttbr1_el12, x4, x5
[^:]*:	d57c2022 	mrrs	x2, x3, ttbr1_el2
[^:]*:	d55c2024 	msrr	ttbr1_el2, x4, x5
[^:]*:	d57c2102 	mrrs	x2, x3, vttbr_el2
[^:]*:	d55c2104 	msrr	vttbr_el2, x4, x5