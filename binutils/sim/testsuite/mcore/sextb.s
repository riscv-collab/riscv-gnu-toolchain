# check that sext.b/sext.h work correctly
# mach: mcore

.include "testutils.inc"

	start
	# Construct -120 using bgeni+addi+sext
	bgeni	r2, 7
	addi	r2,8
	sextb	r2

	# Construct -120 using movi+not
	movi	r7,119
	not	r7

	# Compare them, they should be equal
	cmpne	r2,r7
	jbt	.L1
	pass
.L1:
	fail




