# Check 32bit SM4 instructions

	.text
_start:
	vsm4key4	%ymm4, %ymm5, %ymm6
	vsm4key4	%xmm4, %xmm5, %xmm6
	vsm4key4	0x10000000(%esp, %esi, 8), %ymm5, %ymm6
	vsm4key4	(%ecx), %ymm5, %ymm6
	vsm4key4	0x10000000(%esp, %esi, 8), %xmm5, %xmm6
	vsm4key4	(%ecx), %xmm5, %xmm6
	vsm4rnds4	%ymm4, %ymm5, %ymm6
	vsm4rnds4	%xmm4, %xmm5, %xmm6
	vsm4rnds4	0x10000000(%esp, %esi, 8), %ymm5, %ymm6
	vsm4rnds4	(%ecx), %ymm5, %ymm6
	vsm4rnds4	0x10000000(%esp, %esi, 8), %xmm5, %xmm6
	vsm4rnds4	(%ecx), %xmm5, %xmm6

	.intel_syntax noprefix
	vsm4key4	ymm6, ymm5, ymm4
	vsm4key4	xmm6, xmm5, xmm4
	vsm4key4	ymm6, ymm5, YMMWORD PTR [esp+esi*8+0x10000000]
	vsm4key4	ymm6, ymm5, YMMWORD PTR [ecx]
	vsm4key4	xmm6, xmm5, XMMWORD PTR [esp+esi*8+0x10000000]
	vsm4key4	xmm6, xmm5, XMMWORD PTR [ecx]
	vsm4rnds4	ymm6, ymm5, ymm4
	vsm4rnds4	xmm6, xmm5, xmm4
	vsm4rnds4	ymm6, ymm5, YMMWORD PTR [esp+esi*8+0x10000000]
	vsm4rnds4	ymm6, ymm5, YMMWORD PTR [ecx]
	vsm4rnds4	xmm6, xmm5, XMMWORD PTR [esp+esi*8+0x10000000]
	vsm4rnds4	xmm6, xmm5, XMMWORD PTR [ecx]
