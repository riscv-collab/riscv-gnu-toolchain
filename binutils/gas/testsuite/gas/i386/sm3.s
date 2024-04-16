# Check 32bit SM3 instructions

	.text
_start:
	vsm3msg1	%xmm4, %xmm5, %xmm6	 #SM3
	vsm3msg1	0x10000000(%esp, %esi, 8), %xmm5, %xmm6	 #SM3
	vsm3msg1	(%ecx), %xmm5, %xmm6	 #SM3
	vsm3msg2	%xmm4, %xmm5, %xmm6	 #SM3
	vsm3msg2	0x10000000(%esp, %esi, 8), %xmm5, %xmm6	 #SM3
	vsm3msg2	(%ecx), %xmm5, %xmm6	 #SM3
	vsm3rnds2	$123, %xmm4, %xmm5, %xmm6	 #SM3
	vsm3rnds2	$123, 0x10000000(%esp, %esi, 8), %xmm5, %xmm6	 #SM3
	vsm3rnds2	$123, (%ecx), %xmm5, %xmm6	 #SM3

	.intel_syntax noprefix
	vsm3msg1	xmm6, xmm5, xmm4	 #SM3
	vsm3msg1	xmm6, xmm5, XMMWORD PTR [esp+esi*8+0x10000000]	 #SM3
	vsm3msg1	xmm6, xmm5, XMMWORD PTR [ecx]	 #SM3
	vsm3msg2	xmm6, xmm5, xmm4	 #SM3
	vsm3msg2	xmm6, xmm5, XMMWORD PTR [esp+esi*8+0x10000000]	 #SM3
	vsm3msg2	xmm6, xmm5, XMMWORD PTR [ecx]	 #SM3
	vsm3rnds2	xmm6, xmm5, xmm4, 123	 #SM3
	vsm3rnds2	xmm6, xmm5, XMMWORD PTR [esp+esi*8+0x10000000], 123	 #SM3
	vsm3rnds2	xmm6, xmm5, XMMWORD PTR [ecx], 123	 #SM3
