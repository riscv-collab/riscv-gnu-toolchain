# check that lsri works correctly
# mach: mcore

.include "testutils.inc"

	start
	# Construct -1
	bmaski	r2, 32

	# Clear a couple bits
	bclri r2, 0
	bclri r2, 1

	# rotate by 16
	rotli	r2, 16

	# Construct 0xfffcffff
	bmaski	r1, 32
	bclri r1, 16
	bclri r1, 17

	# Compare them, they should be equal
	cmpne	r2,r1
	jbt	.L1
	pass
.L1:
	fail




