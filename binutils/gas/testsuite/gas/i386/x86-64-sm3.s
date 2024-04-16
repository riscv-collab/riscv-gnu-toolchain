# Check 64bit SM3 instructions

	.text
_start:
	vsm3msg1	%xmm14, %xmm5, %xmm6	 #SM3
	vsm3msg1	0x10000000(%rbp, %r14, 8), %xmm15, %xmm6	 #SM3
	vsm3msg1	(%r9), %xmm15, %xmm6	 #SM3
	vsm3msg2	%xmm14, %xmm5, %xmm6	 #SM3
	vsm3msg2	0x10000000(%rbp, %r14, 8), %xmm15, %xmm6	 #SM3
	vsm3msg2	(%r9), %xmm15, %xmm6	 #SM3
	vsm3rnds2	$123, %xmm14, %xmm5, %xmm6	 #SM3
	vsm3rnds2	$123, 0x10000000(%rbp, %r14, 8), %xmm15, %xmm6	 #SM3
	vsm3rnds2	$123, (%r9), %xmm15, %xmm6	 #SM3

	.intel_syntax noprefix
	vsm3msg1	xmm6, xmm5, xmm14	 #SM3
	vsm3msg1	xmm6, xmm15, XMMWORD PTR [rbp+r14*8+0x10000000]	 #SM3
	vsm3msg1	xmm6, xmm15, XMMWORD PTR [r9]	 #SM3
	vsm3msg2	xmm6, xmm5, xmm14	 #SM3
	vsm3msg2	xmm6, xmm15, XMMWORD PTR [rbp+r14*8+0x10000000]	 #SM3
	vsm3msg2	xmm6, xmm15, XMMWORD PTR [r9]	 #SM3
	vsm3rnds2	xmm6, xmm5, xmm14, 123	 #SM3
	vsm3rnds2	xmm6, xmm15, XMMWORD PTR [rbp+r14*8+0x10000000], 123	 #SM3
	vsm3rnds2	xmm6, xmm15, XMMWORD PTR [r9], 123	 #SM3
