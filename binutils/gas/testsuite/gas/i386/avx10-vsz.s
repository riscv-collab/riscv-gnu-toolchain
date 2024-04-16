# Test AVX10 vector size restriction
	.text

	.irp isa, default, .avx10.1/256, .avx10.1/128, .avx10.1

	.att_syntax prefix
	.warning "\isa"
	.arch generic32
	.arch \isa

	vcvtpd2phz	(%eax), %xmm0
	vcvtpd2phy	(%eax), %xmm0
	vcvtpd2ph	(%eax){1to8}, %xmm0
	vcvtpd2ph	(%eax){1to4}, %xmm0

	vcvtpd2ps	(%eax), %ymm0
	vcvtpd2psy	(%eax), %xmm0
	vcvtpd2psy	(%eax), %xmm0{%k1}
	vcvtpd2ps	(%eax){1to4}, %xmm0

	vfpclasspsy	$0, (%eax), %k0
	vfpclasspsz	$0, (%eax), %k0

	.intel_syntax noprefix
	vcvtpd2ph	xmm0, [eax]
	vcvtpd2ps	xmm0, [eax]
	vfpclassps	k0, [eax], 0

	.endr

	.p2align 4
