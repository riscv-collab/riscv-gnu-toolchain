# The bundles in this file all have 8 syllables.

	addd $r0 = $r0, 123456789010	# 1 ALU + 2 Immediate Extensions
	addd $r0 = $r0, 123456789010	# 1 ALU + 2 Immediate Extensions
	addd $r1 = $r2, 1234		# 1 ALU + 1 Immediate Extension
	;;
	igoto $r0			# 1 BCU
	xmt44d $a0a1a2a3 = $a0a1a2a3	# 1 TCA
	addd $r0 = $r0, 1234		# 1 ALU + 1 Immediate Extension
	addd $r0 = $r0, 12345678901	# 1 ALU + 1 Immediate Extension
	fmuld $r1 = $r2, $r3		# 1 MAU
	lwz $r0 = 0[$r0]		# 1 LSU
	;;
