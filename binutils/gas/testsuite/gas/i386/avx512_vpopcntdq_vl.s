# Check 32bit AVX512VL+VPOPCNTDQ instructions

	.text
vpopcnt:
	vpopcntd	%ymm5, %ymm6
	vpopcntd	%ymm5, %ymm6{%k7}
	vpopcntd	%ymm5, %ymm6{%k7}{z}
	vpopcntd	(%ecx), %ymm6
	vpopcntd	-123456(%esp,%esi,8), %ymm6
	vpopcntd	(%eax){1to8}, %ymm6
	vpopcntd	4064(%edx), %ymm6	 #  Disp8
	vpopcntd	4096(%edx), %ymm6
	vpopcntd	-4096(%edx), %ymm6	 #  Disp8
	vpopcntd	-4128(%edx), %ymm6
	vpopcntd	508(%edx){1to8}, %ymm6	 #  Disp8
	vpopcntd	512(%edx){1to8}, %ymm6
	vpopcntd	-512(%edx){1to8}, %ymm6	 #  Disp8
	vpopcntd	-516(%edx){1to8}, %ymm6
	vpopcntq	%ymm5, %ymm6
	vpopcntq	%ymm5, %ymm6{%k7}
	vpopcntq	%ymm5, %ymm6{%k7}{z}
	vpopcntq	(%ecx), %ymm6
	vpopcntq	-123456(%esp,%esi,8), %ymm6
	vpopcntq	(%eax){1to4}, %ymm6
	vpopcntq	4064(%edx), %ymm6	 #  Disp8
	vpopcntq	4096(%edx), %ymm6
	vpopcntq	-4096(%edx), %ymm6	 #  Disp8
	vpopcntq	-4128(%edx), %ymm6
	vpopcntq	1016(%edx){1to4}, %ymm6	 #  Disp8
	vpopcntq	1024(%edx){1to4}, %ymm6
	vpopcntq	-1024(%edx){1to4}, %ymm6	 #  Disp8
	vpopcntq	-1032(%edx){1to4}, %ymm6

	.intel_syntax noprefix
	vpopcntd	xmm6, xmm5
	vpopcntd	xmm6{k7}, xmm5
	vpopcntd	xmm6{k7}{z}, xmm5
	vpopcntd	xmm6, XMMWORD PTR [ecx]
	vpopcntd	xmm6, XMMWORD PTR [esp+esi*8-123456]
	vpopcntd	xmm6, [eax]{1to4}
	vpopcntd	xmm6, DWORD BCST [eax]
	vpopcntd	xmm6, XMMWORD PTR [edx+2032]	 #  Disp8
	vpopcntd	xmm6, XMMWORD PTR [edx+2048]
	vpopcntd	xmm6, XMMWORD PTR [edx-2048]	 #  Disp8
	vpopcntd	xmm6, XMMWORD PTR [edx-2064]
	vpopcntd	xmm6, [edx+508]{1to4}	 #  Disp8
	vpopcntd	xmm6, [edx+512]{1to4}
	vpopcntd	xmm6, [edx-512]{1to4}	 #  Disp8
	vpopcntd	xmm6, [edx-516]{1to4}
	vpopcntq	xmm6, xmm5
	vpopcntq	xmm6{k7}, xmm5
	vpopcntq	xmm6{k7}{z}, xmm5
	vpopcntq	xmm6, XMMWORD PTR [ecx]
	vpopcntq	xmm6, XMMWORD PTR [esp+esi*8-123456]
	vpopcntq	xmm6, [eax]{1to2}
	vpopcntq	xmm6, QWORD BCST [eax]
	vpopcntq	xmm6, XMMWORD PTR [edx+2032]	 #  Disp8
	vpopcntq	xmm6, XMMWORD PTR [edx+2048]
	vpopcntq	xmm6, XMMWORD PTR [edx-2048]	 #  Disp8
	vpopcntq	xmm6, XMMWORD PTR [edx-2064]
	vpopcntq	xmm6, [edx+1016]{1to2}	 #  Disp8
	vpopcntq	xmm6, [edx+1024]{1to2}
	vpopcntq	xmm6, [edx-1024]{1to2}	 #  Disp8
	vpopcntq	xmm6, [edx-1032]{1to2}
