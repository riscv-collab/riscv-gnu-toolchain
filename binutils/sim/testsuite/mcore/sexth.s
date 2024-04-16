# check that sext.b/sext.h work correctly
# mach: mcore

.include "testutils.inc"

	start
	# Construct -32760 using bgeni+addi+sext
	bgeni	r2, 15
	addi	r2,8
	sexth	r2

	# Construct -32760 using bmask+subi+not
        bmaski  r7,15
        subi    r7,8    // 32759 0x7ff7
	not	r7


	# Compare them, they should be equal
	cmpne	r2,r7
	jbt	.L1
	pass
.L1:
	fail




