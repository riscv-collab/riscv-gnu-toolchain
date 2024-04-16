# Check 64bit SM4 instructions

	.text
_start:
	vsm4key4	%ymm14, %ymm5, %ymm6 
	vsm4key4	%xmm14, %xmm5, %xmm6
	vsm4key4	0x10000000(%rbp, %r14, 8), %ymm15, %ymm6
	vsm4key4	(%r9), %ymm15, %ymm6
	vsm4key4	0x10000000(%rbp, %r14, 8), %xmm15, %xmm6
	vsm4key4	(%r9), %xmm15, %xmm6
	vsm4rnds4	%ymm14, %ymm5, %ymm6
	vsm4rnds4	%xmm14, %xmm5, %xmm6
	vsm4rnds4	0x10000000(%rbp, %r14, 8), %ymm15, %ymm6
	vsm4rnds4	(%r9), %ymm15, %ymm6
	vsm4rnds4	0x10000000(%rbp, %r14, 8), %xmm15, %xmm6
	vsm4rnds4	(%r9), %xmm15, %xmm6

	.intel_syntax noprefix
	vsm4key4	ymm6, ymm5, ymm14
	vsm4key4	xmm6, xmm5, xmm14
	vsm4key4	ymm6, ymm15, YMMWORD PTR [rbp+r14*8+0x10000000]
	vsm4key4	ymm6, ymm15, YMMWORD PTR [r9]
	vsm4key4	xmm6, xmm15, XMMWORD PTR [rbp+r14*8+0x10000000]
	vsm4key4	xmm6, xmm15, XMMWORD PTR [r9]
	vsm4rnds4	ymm6, ymm5, ymm14
	vsm4rnds4	xmm6, xmm5, xmm14
	vsm4rnds4	ymm6, ymm15, YMMWORD PTR [rbp+r14*8+0x10000000]
	vsm4rnds4	ymm6, ymm15, YMMWORD PTR [r9]
	vsm4rnds4	xmm6, xmm15, XMMWORD PTR [rbp+r14*8+0x10000000]
	vsm4rnds4	xmm6, xmm15, XMMWORD PTR [r9]
