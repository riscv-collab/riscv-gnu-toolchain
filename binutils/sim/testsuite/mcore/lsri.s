# check that lsri works correctly
# mach: mcore

.include "testutils.inc"

	start
	# Construct -1
	bmaski	r2, 32

	# logical shift right by 24
	lsri	r2, 24

	# Construct 255
	bmaski	r1, 8

	# Compare them, they should be equal
	cmpne	r2,r1
	jbt	.L1
	pass
.L1:
	fail




