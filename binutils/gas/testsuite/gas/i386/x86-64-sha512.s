# Check 64bit SHA512 instructions

	.text
_start:
	vsha512msg1	%xmm15, %ymm6	 #SHA512
	vsha512msg2	%ymm5, %ymm15	 #SHA512
	vsha512rnds2	%xmm4, %ymm5, %ymm14	 #SHA512

	.intel_syntax noprefix
	vsha512msg1	ymm6, xmm15	 #SHA512
	vsha512msg2	ymm15, ymm5	 #SHA512
	vsha512rnds2	ymm14, ymm5, xmm4	 #SHA512
