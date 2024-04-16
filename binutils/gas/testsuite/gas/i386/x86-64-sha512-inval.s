# Check Illegal SHA512 instructions

	.text
_start:
	vsha512msg1	(%ecx), %ymm6
	vsha512msg2	(%ecx), %ymm6
	vsha512rnds2	(%ecx), %ymm5, %ymm6
