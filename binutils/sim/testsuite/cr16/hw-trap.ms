# mach(): cr16

	.include "testutils.inc"

	start

# perform trap
	movw $0,r2
	movw $0x410,r0
	pass # the pass macro use the trap 8
