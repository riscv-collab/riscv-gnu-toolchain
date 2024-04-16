# check that lsr works correctly
# mach: mcore

.include "testutils.inc"

	start
	# Construct -1
	bmaski	r2, 32

	# Construct 24
	movi r3, 24

	# logical shift right by r3 (24)
	lsr	r2, r3

	# Construct 255
	bmaski	r1, 8

	# Compare them, they should be equal
	cmpne	r2,r1
	jbt	.L1
	pass
.L1:
	fail




