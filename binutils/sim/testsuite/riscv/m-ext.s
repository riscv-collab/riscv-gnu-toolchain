# Check that the RV32M instructions run without any faults.
# mach: riscv

.include "testutils.inc"

	start

	.option	arch, +m
	mul	x0, x1, x2
	mulh	x0, x1, x2
	mulhu	x0, x1, x2
	mulhsu	x0, x1, x2
	div	x0, x1, x2
	divu	x0, x1, x2
	rem	x0, x1, x2
	remu	x0, x1, x2

	pass
