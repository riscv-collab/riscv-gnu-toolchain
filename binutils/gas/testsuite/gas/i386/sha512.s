# Check 32bit SHA512 instructions

	.text
_start:
	vsha512msg1	%xmm5, %ymm6	 #SHA512
	vsha512msg2	%ymm5, %ymm6	 #SHA512
	vsha512rnds2	%xmm4, %ymm5, %ymm6	 #SHA512

	.intel_syntax noprefix
	vsha512msg1	ymm6, xmm5	 #SHA512
	vsha512msg2	ymm6, ymm5	 #SHA512
	vsha512rnds2	ymm6, ymm5, xmm4	 #SHA512
