	.text

	.irp isa, default, .noavx512vl, .noavx512f, .avx_vnni, .avx_ifma, .avx_ne_convert

	.arch default
	.arch \isa

	vpdpbusd	%ymm0, %ymm1, %ymm2
	vpdpbusd	0x20(%eax), %ymm1, %ymm2
	vpdpbusd	0x100(%eax), %ymm1, %ymm2

	vpmadd52luq	%ymm0, %ymm1, %ymm2
	vpmadd52luq	0x20(%eax), %ymm1, %ymm2
	vpmadd52luq	0x100(%eax), %ymm1, %ymm2
	vpmadd52luq	(%eax){1to4}, %ymm1, %ymm2

	vcvtneps2bf16	%ymm0, %xmm1
	vcvtneps2bf16y	%ymm0, %xmm1
	vcvtneps2bf16y	0x20(%eax), %xmm1
	vcvtneps2bf16y	0x100(%eax), %xmm1
	vcvtneps2bf16y	(%eax){1to8}, %xmm1

	.intel_syntax noprefix
	vcvtneps2bf16	xmm0, xmmword ptr [ecx]
	vcvtneps2bf16	xmm0, ymmword ptr [ecx]
	.att_syntax prefix

	.endr
